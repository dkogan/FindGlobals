#include <stdio.h>
#include <stdlib.h>
#include <elfutils/libdwfl.h>
#include <dwarf.h>
#include <inttypes.h>
#include <string.h>



/*
Some of this has been lifted from dwarf_prototypes.c in ltrace. */


#define confirm(cond, format, ...) do {                                 \
        bool _cond = cond;                                              \
        if(!_cond) {                                                    \
            fprintf(stderr, "%s(): "format"\n", __func__, ##__VA_ARGS__); \
            result = false;                                             \
            goto done;                                                  \
        }                                                               \
} while(0)

#define confirm_with_die(die, cond, format, ...) do {                   \
        bool _cond = cond;                                              \
        if(!_cond) {                                                    \
            fprintf(stderr, "%s(): die '%s' @ 0x%" PRIx64 ": "format"\n", \
                    __func__, dwarf_diename(die), dwarf_dieoffset(die), \
                    ##__VA_ARGS__);                                     \
            result = false;                                             \
            goto done;                                                  \
        }                                                               \
    } while(0)



static bool get_size(Dwarf_Die* die, unsigned int* out)
{
    bool result = true;

    Dwarf_Die sub_die;
    Dwarf_Attribute attr;

    // if we have a DW_AT_specification then I get the size information from
    // THAT node
    if( dwarf_attr(die, DW_AT_specification, &attr) )
    {
        dwarf_formref_die(&attr, &sub_die);
        return get_size(&sub_die, out);
    }
    if( dwarf_attr(die, DW_AT_abstract_origin, &attr) )
    {
        dwarf_formref_die(&attr, &sub_die);
        return get_size(&sub_die, out);
    }

    // No DW_AT_specification. Get the type data from this node
    confirm_with_die(die, dwarf_attr(die, DW_AT_type, &attr),
                     "Variable doesn't have DW_AT_type");
    dwarf_formref_die(&attr, &sub_die);

    Dwarf_Word size;
    dwarf_aggregate_size(&sub_die, &size);
    *out = (unsigned int)size;

 done:
    return result;
}

static bool get_location(Dwarf_Die* die, void** out)
{
    Dwarf_Attribute attr;

    // defaults
    bool result = true;
    *out = NULL;

    if( !dwarf_attr(die, DW_AT_location, &attr) )
        // No location. This is a (for instance) a stack variable in a
        // function. It doesn't persist, so I don't care about it
        return true;

    // elfutils/tests/varlocs.c has an example of this. I implement a very
    // small subset
    Dwarf_Op* op;
    size_t oplen;
    confirm_with_die( die, 0 == dwarf_getlocation(&attr, &op, &oplen),
                      "Couldn't dwarf_getlocation()");

    // I only accept simple, directly-given addresses
    if( oplen != 1 )               goto done;
    if( op[0].atom != DW_OP_addr ) goto done;
    *out = (void*)op[0].number;

 done:
    return result;
}

static bool process_die_children(Dwarf_Die *parent, const char* source_pattern)
{
    Dwarf_Die die;
    if (dwarf_child(parent, &die) != 0) {
        // no child nodes, so nothing to do
        return true;
    }

    bool result = true;
    int done = 0;

    for(; !done;

        // dwarf_siblingof() returns 0 if there's a sibling, <0 on error and >0
        // if no sibling exists. I'm done if !0.
        ({ done = dwarf_siblingof(&die, &die);
           confirm_with_die( &die, done >= 0, "dwarf_siblingof() failed" );
        }))
    {
        if( dwarf_tag(&die) == DW_TAG_subprogram )
        {
            // looking at a function. recurse down to find static variables
            if(!process_die_children(&die, NULL))
                return false;
            continue;
        }

        if( dwarf_tag(&die) != DW_TAG_variable )
            continue;

        const char* decl_file = dwarf_decl_file(&die);
        if( !decl_file )
            continue;

        if( source_pattern && !strstr( decl_file, source_pattern) )
            continue;

        const char* var_name = dwarf_diename(&die);

        unsigned int size;
        if(! get_size(&die, &size) )
        {
            result = false;
            goto done;
        }
        if( size == 0 )
            continue;

        void* addr;
        if(!get_location(&die, &addr))
        {
            result = false;
            goto done;
        }
        if( addr == NULL )
            continue;

        fprintf(stderr, "DWARF says %s at %p, size %d\n", var_name, addr, size);
    }

 done:
    return result;
}


bool get_addrs(const char* executable_filename, const char* source_pattern)
{
    Dwfl*        dwfl        = NULL;
    Dwfl_Module* dwfl_module = NULL;

    bool result = true;

    const Dwfl_Callbacks proc_callbacks =
        {
            .find_elf       = dwfl_linux_proc_find_elf,
            .find_debuginfo = dwfl_standard_find_debuginfo
        };
    confirm(NULL != (dwfl =
                     dwfl_begin(&proc_callbacks)),
            "Error calling dwfl_begin()");

    dwfl_report_begin(dwfl);
    confirm( NULL !=
             (dwfl_module =
              dwfl_report_elf(dwfl,
                              executable_filename,
                              executable_filename, -1,
                              0, false)),
             "Error calling dwfl_report_begin()");
    dwfl_report_end(dwfl, NULL, NULL);

    Dwarf_Addr bias;
    Dwarf_Die* die = NULL;
    while( NULL != (die = dwfl_module_nextcu(dwfl_module, die, &bias)) )
        if (dwarf_tag(die) == DW_TAG_compile_unit)
        {
            if(!process_die_children(die, source_pattern))
            {
                result = false;
                break;
            }
        }
        else
            // Really expecting a DW_TAG_compile_unit. I guess I skip this.
            // Maybe this is an error
        {}

 done:
    if(dwfl != NULL)
        dwfl_end(dwfl);
    return result;
}
