#include <stdio.h>
#include <stdlib.h>
#include <elfutils/libdwfl.h>
#include <dwarf.h>
#include <libelf.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>


#define confirm(cond, format, ...) do {                                 \
        bool _cond = cond;                                              \
        if(!_cond) {                                                    \
            fprintf(stderr, "%s(): "format"\n", __func__, ##__VA_ARGS__); \
            goto done;                                                  \
        }                                                               \
} while(0)

#define confirm_with_die(die, cond, format, ...) do {                   \
        bool _cond = cond;                                              \
        if(!_cond) {                                                    \
            fprintf(stderr, "%s(): die '%s' @ 0x%" PRIx64 ": "format"\n", \
                    __func__, dwarf_diename(die), dwarf_dieoffset(die), \
                    ##__VA_ARGS__);                                     \
            goto done;                                                  \
        }                                                               \
    } while(0)




#define DEBUG

#ifdef DEBUG
 #define DEBUGLOG(fmt, ...) fprintf(stderr, fmt "\n", __VA_ARGS__)
#else
 #define DEBUGLOG(fmt, ...) do { } while(0)
#endif


#define MAX_WRITEABLE_RANGES 10
static struct memrange_t
{
    void* start;
    void* end;
} writeable_memory[MAX_WRITEABLE_RANGES];
static int Nwriteable_memory = 0;



static bool get_size(Dwarf_Die* die, unsigned int* out)
{
    bool result = false;

    Dwarf_Die sub_die;
    Dwarf_Attribute attr;

    // if we have an indirect type reference then I get the size information
    // from THAT node
    if( dwarf_attr(die, DW_AT_specification,   &attr) ||
        dwarf_attr(die, DW_AT_abstract_origin, &attr) )
    {
        dwarf_formref_die(&attr, &sub_die);
        return get_size(&sub_die, out);
    }

    confirm_with_die(die, dwarf_attr(die, DW_AT_type, &attr),
                     "Variable doesn't have DW_AT_type");
    dwarf_formref_die(&attr, &sub_die);

    Dwarf_Word size;

    // I sometimes see errors when querying the variable size of length-0
    // objects. The ones I've seen are triggered by building with -ffast-math
    // and looking at size-0 objects like x0 in the demo application source
    // tst2.c. In that case I simply return size=0
    if( dwarf_aggregate_size(&sub_die, &size) )
        size = 0;
    // confirm_with_die(&sub_die, 0 == dwarf_aggregate_size(&sub_die, &size),
    //                  "Couldn't get size");

    *out = (unsigned int)size;
    result = true;

 done:
    return result;
}

static bool get_location(Dwarf_Die* die, void** out)
{
    Dwarf_Attribute attr;

    // defaults
    bool result = false;
    *out = NULL;

    if( !dwarf_attr(die, DW_AT_location, &attr) )
        // No location. This is a (for instance) a stack variable in a
        // function. It doesn't persist, so I don't care about it
        return true;

    // elfutils/tests/varlocs.c has an example of this. I implement a very
    // small subset
    Dwarf_Op* op;
    size_t oplen;
    if( 0 != dwarf_getlocation(&attr, &op, &oplen) )
        goto done;

    // I only accept simple, directly-given addresses. Anything more complicated
    // is a successful no-address return
    if( oplen != 1 ||
        op[0].atom != DW_OP_addr )
    {
        result = true;
        goto done;
    }
    *out = (void*)op[0].number;
    result = true;

 done:
    return result;
}

static bool is_addr_writeable( const void* addr,

                               int Nwriteable_memory,
                               const struct memrange_t* writeable_memory )
{
    for(int i=0; i<Nwriteable_memory; i++)
        if( addr >= writeable_memory[i].start &&
            addr <  writeable_memory[i].end )
            return true;

    return false;
}





static bool process_die_children(Dwarf_Die *parent, const char* source_pattern, Dwarf_Addr bias)
{
    Dwarf_Die die;
    if (dwarf_child(parent, &die) != 0) {
        // no child nodes, so nothing to do
        return true;
    }

    bool result = false;
    int  done   = 0;

    for(; !done;

        // dwarf_siblingof() returns 0 if there's a sibling, <0 on error and >0
        // if no sibling exists. I'm done if !0.
        ({ done = dwarf_siblingof(&die, &die);
           confirm_with_die( &die, done >= 0, "dwarf_siblingof() failed" );
        }))
    {

#if 0 // for verbose debugging
        DEBUGLOG("looking at die %#010x: %s",
                 (unsigned)dwarf_dieoffset(&die),
                 dwarf_diename(&die));
#endif

        int tag = dwarf_tag(&die);
        if( tag == DW_TAG_subprogram || tag == DW_TAG_module )
        {
            // looking at a function. recurse down to find static variables
            if(!process_die_children(&die, NULL, bias))
                return false;
            continue;
        }

        if( tag != DW_TAG_variable )
            continue;

        const char* decl_file = dwarf_decl_file(&die);
        if( !decl_file )
            continue;

        DEBUGLOG("looking at var '%s' in file '%s' matching pattern '%s'",
                 dwarf_diename(&die), decl_file, source_pattern ? source_pattern : "(nil)");
        if( source_pattern && !strstr( decl_file, source_pattern) )
            continue;

        const char* var_name = dwarf_diename(&die);

        void* addr;
        if(!get_location(&die, &addr))
        {
            // I couldn't get the location for some reason. Normally I'd simply
            // give up (goto done), but I keep going to get the subsequent
            // entries
            continue;
        }
        if( addr == NULL )
            continue;

        unsigned int size;
        if(! get_size(&die, &size) )
            goto done;
        if( size == 0 )
            continue;


        if( !is_addr_writeable(addr + bias, Nwriteable_memory, writeable_memory) )
            DEBUGLOG("readonly %s at %p, size %d", var_name, addr + bias, size);
        else
            printf("%s at %p, size %d\n", var_name, addr + bias, size);
    }

    result = true;

 done:
    return result;
}

static bool get_writeable_memory_ranges(Dwfl_Module* dwfl_module,
                                        int* Nwriteable_memory,
                                        struct memrange_t* writeable_memory,
                                        int Nwriteable_memory_max)
{
    bool result = false;

    GElf_Addr elf_bias;
    Elf* elf = dwfl_module_getelf (dwfl_module, &elf_bias);
    DEBUGLOG("elf bias: %#lx", elf_bias);
    confirm( NULL != elf,
             "Error getting the Elf object from dwfl");

    size_t num_phdr;
    confirm( 0 == elf_getphdrnum(elf, &num_phdr),
            "Error getting the program header count from ELF");

    Elf64_Phdr* phdr = elf64_getphdr(elf);
    confirm( phdr, "Error getting program headers from ELF");

    *Nwriteable_memory = 0;
    for( size_t i=0; i<num_phdr; i++ )
    {
        // I'm only looking for readable,writeable segments that have data
        if( ! (phdr[i].p_type == PT_LOAD &&
               phdr[i].p_flags & PF_W    &&
               phdr[i].p_flags & PF_R) )
            continue;

        if( phdr[i].p_memsz <= 0 )
            continue;
        confirm( *Nwriteable_memory < Nwriteable_memory_max,
                 "Too many writeable memory segments to fit into my buffer");

        writeable_memory[(*Nwriteable_memory)++] =
            (struct memrange_t){ .start = (void*)(elf_bias + phdr[i].p_vaddr),
                                 .end   = (void*)(elf_bias + phdr[i].p_vaddr + phdr[i].p_memsz) };

        DEBUGLOG("See writeable memory at %p-%p of size %#lx (in-file size %#lx)",
                 (void*)(elf_bias + phdr[i].p_vaddr),
                 (void*)(elf_bias + phdr[i].p_vaddr + phdr[i].p_memsz),
                 phdr[i].p_memsz,
                 phdr[i].p_filesz);
    }
    result = true;

 done:
    return result;
}

bool get_addrs(void (*func)(void),
               const char* source_pattern)
{
    Dwfl*        dwfl        = NULL;
    Dwfl_Module* dwfl_module = NULL;

    bool result = false;

    const Dwfl_Callbacks proc_callbacks =
        {
            .find_elf       = dwfl_linux_proc_find_elf,
            .find_debuginfo = dwfl_standard_find_debuginfo
        };
    confirm(NULL != (dwfl =
                     dwfl_begin(&proc_callbacks)),
            "Error calling dwfl_begin()");

    int report_result = dwfl_linux_proc_report (dwfl, getpid());
    DEBUGLOG("dwfl_linux_proc_report says: %d", report_result );

    dwfl_module = dwfl_addrmodule(dwfl, (Dwarf_Addr)func);
    if(dwfl_module == NULL)
    {
        DEBUGLOG("Couldn't find dwfl module containing function %p", (void*)func);
        return false;
    }

    if(!get_writeable_memory_ranges(dwfl_module,
                                    &Nwriteable_memory, writeable_memory,
                                    sizeof(writeable_memory)/sizeof(writeable_memory[0])))
        goto done;




    Dwarf_Addr bias;
    Dwarf_Die* die = NULL;
    while( NULL != (die = dwfl_module_nextcu(dwfl_module, die, &bias)) )
        if (dwarf_tag(die) == DW_TAG_compile_unit)
        {
            DEBUGLOG("CU bias: %#lx", bias);
            if(!process_die_children(die, source_pattern, bias))
                goto done;
        }
        else
            // Really expecting a DW_TAG_compile_unit. I guess I skip this.
            // Maybe this is an error
        {
            DEBUGLOG("Expected compile_unit, but got %d...", (int)dwarf_tag(die));
        }

    result = true;

 done:
    if(dwfl != NULL)
        dwfl_end(dwfl);
    return result;
}
