OBJECTS_TST       := $(addsuffix .o,$(basename $(wildcard tst*.c)))
OBJECT_MAIN       := main.o
OBJECT_GETGLOBALS := getglobals.o

LDLIBS += -ldw -lelf

# if any -O... is requested, use that; otherwise, do -O3
FLAGS_OPTIMIZATION := $(if $(filter -O%,$(CFLAGS) $(CXXFLAGS) $(CPPFLAGS)),,-O3 -ffast-math)
CPPFLAGS := -MMD -g $(FLAGS_OPTIMIZATION) -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-variable
CFLAGS += -std=gnu11
CXXFLAGS += -std=gnu++11


$(OBJECT_GETGLOBALS) $(OBJECTS_TST): CPPFLAGS += -fPIC


all: getglobals getglobals_viaso show_globals_from_elf_file

getglobals.so: $(OBJECT_GETGLOBALS)
	gcc -shared -o $@ $^ -ldw -lelf

tst.so: $(OBJECTS_TST)
	gcc -shared -o $@ $^

getglobals: $(OBJECTS_TST) $(OBJECT_GETGLOBALS) $(OBJECT_MAIN)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

getglobals_viaso: $(OBJECT_MAIN) getglobals.so tst.so
	$(CC) -Wl,-rpath=$(abspath .) $(LDFLAGS) -o $@ $^ $(LDLIBS)

show_globals_from_elf_file: show_globals_from_elf_file.o $(OBJECT_GETGLOBALS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)


clean:
	rm -rf getglobals *.o *.d *.so
.PHONY: clean

-include *.d
