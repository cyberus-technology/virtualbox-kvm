
FS=hfs

SOURCES += ../fsw_core.c
SOURCES += ../fsw_$(FS).c
SOURCES += ../fsw_lib.c
SOURCES += fsw_posix.c

OBJ = ${foreach src,$(SOURCES),$(addsuffix .o, $(basename $(src)))}

#lslr.c
#lsroot.c
CPPFLAGS += -DHOST_POSIX
CPPFLAGS += -DFSTYPE=$(FS) 
ASFLAGS += -include asm.h 
CPPFLAGS += -I.. 
CPPFLAGS += -I. 
CFLAGS += -g -gdwarf-2
CFLAGS += -m32

.PHONY: all clear
all:lslr lsroot
clean:
	$(RM) $(OBJ) lslr lsroot lslr.o lsroot.o

lslr: OBJ += lslr.o
lslr: $(OBJ)

lsroot: OBJ += lsroot.o
lsroot: $(OBJ)


