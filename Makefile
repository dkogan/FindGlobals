SOURCES := $(wildcard *.c *.cpp *.cc)
OBJECTS := $(addsuffix .o,$(basename $(SOURCES)))

LDLIBS += -ldw -lelf

# if any -O... is requested, use that; otherwise, do -O3
FLAGS_OPTIMIZATION := $(if $(filter -O%,$(CFLAGS) $(CXXFLAGS) $(CPPFLAGS)),,-O3 -ffast-math)
CPPFLAGS := -MMD -g $(FLAGS_OPTIMIZATION) -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
CFLAGS += -std=gnu11
CXXFLAGS += -std=gnu++11


CPPFLAGS += -fPIC


all: getglobals getglobals_viaso

getglobals.so: getglobals.o
	gcc -shared -o $@ $^ -ldw -lelf

getglobals: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

getglobals_viaso: $(OBJECTS) getglobals.so
	$(CC) -Wl,-rpath=$(shell pwd) $(LDFLAGS) -o $@ $(OBJECTS:getglobals.o=getglobals.so) $(LDLIBS)


clean:
	rm -rf getglobals *.o *.d *.so
.PHONY: clean

-include *.d
