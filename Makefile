#TARGET_DEBUG = 1

##################################################################
#CFLAGS_DEBUG= -gstabs
CFLAGS_DEBUG= -gdwarf-2

WARNINGS = -Wall -Wextra
WARNINGS += -Wwrite-strings -Wcast-qual -Wpointer-arith -Wsign-compare
WARNINGS += -Wundef
WARNINGS += -Wmissing-declarations

#CFLAGS_OPTIM= -O6
#CFLAGS_NO_CYGWIN_DLL=-mno-cygwin
CC       = gcc
#CC       = i686-pc-mingw32-gcc.exe
LL       = gcc
LD       = ld
#LL       = i686-pc-mingw32-gcc.exe
CFLAGS   = $(WARNINGS) $(CFLAGS_OPTIM) -Wall $(CFLAGS_DEBUG) $(CFLAGS_NO_CYGWIN_DLL)
ifneq "$(TARGET_DEBUG)" ""
CFLAGS  += -DTEST_UCAN=1
endif
#CFLAGS  += -I/usr/include/SDL
CPPFLAGS   = $(CFLAGS) -fno-rtti

#LDFLAGS_STRIP_DEVUG_INFO = -s
# LDFLAGS  = -s  $(CFLAGS_NO_CYGWIN_DLL) -lcrypto -lssl #-lsocket -lnsl
LDFLAGS  = $(LDFLAGS_STRIP_DEVUG_INFO)  $(CFLAGS_NO_CYGWIN_DLL) #-lsocket -lnsl
#LDFLAGS += -L./pc-libs
LDFLAGS += -Wl,--enable-stdcall-fixup
#LDFLAGS += --enable-stdcall-fixup
#LDFLAGS += --disable-stdcall-fixup
LDFLAGS += -L./lib
#LDLIBS := -lcygwin
LDLIBS :=
#LDLIBS += -lEcuTalk
#LDLIBS += -lltdl
#LDLIBS += -lcygwin
#LDLIBS += -lSDL
#it's necessary for the new SDL library
#LDLIBS += -lwinmm
#LDLIBS += -lmsvcr70
#LDLIBS += -lmsvcrt
#LDLIBS += -lgdi32
#LDLIBS += -ldxguid
#LDLIBS += -lstdc++
MAKEFILE=Makefile
##################################################################
#TARGET=uCAN_PCAN
#TARGET=uCAN_WeCAN
#TARGET=uCAN_Vector
TARGET=uCAN_All
TARGET_DIR=obj/
LIB_DIR=lib/

#CPPFILES = $(TARGET).cpp
#CPPFILES+= TestList.cpp
#CPPFILES+= debug.cpp
#CPPFILES+= osm_xml.cpp
CFILES    = $(TARGET).c
#CFILES   += xxx.c

#SUBDIRS := yyy

VPATH := $(SUBDIRS) $(TARGET_DIR)
OBJECTS = $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
DEPFILES = $(addprefix $(TARGET_DIR),$(CPPFILES:.cpp=.d) $(CFILES:.c=.d))
INCDIRS = $(addprefix -I./,$(SUBDIRS))
INCDIRS += -I.
CFLAGS += $(INCDIRS)
ifeq "$(TARGET_DEBUG)" ""
LDLIBS += -l$(TARGET)_use
endif

DUMMY_DIR_FILE := $(TARGET_DIR)dummy
DUMMY_LIB_DIR_FILE := $(LIB_DIR)dummy

ifeq "$(TARGET_DEBUG)" ""
TARGET_EXE = $(TARGET_DIR)uCAN_dll_test.exe
TARGET_EXE_DEP = $(TARGET_DLL_USE)
else
TARGET_EXE = $(TARGET_DIR)$(TARGET).exe
endif
TARGET_EXE_OBJECT = $(notdir $(TARGET_EXE:.exe=.o))
TARGET_DLL = $(TARGET_DIR)$(TARGET).dll
TARGET_DLL_USE = $(LIB_DIR)lib$(TARGET)_use.a
TARGET_DLL_DEF = $(LIB_DIR)lib$(TARGET)_use.def

ifeq "$(TARGET_DEBUG)" ""
all : $(TARGET_EXE) $(TARGET_DLL) $(TARGET_DLL_DEF)
else
all : $(TARGET_EXE)
endif

#$(TARGET_EXE) : $(OBJECTS)
$(TARGET_EXE) : $(TARGET_EXE_OBJECT) $(TARGET_EXE_DEP)
	$(LL) $(LDFLAGS) $(addprefix $(TARGET_DIR),$(TARGET_EXE_OBJECT)) $(LDLIBS) -o $@
#	$(LL) $(LDFLAGS) $(addprefix $(TARGET_DIR),$(OBJECTS)) $(LDLIBS) -o $@

$(TARGET_DLL_USE): $(TARGET_DLL)

$(TARGET_DLL): $(OBJECTS) $(DUMMY_LIB_DIR_FILE)
	$(LL) -shared -o $@ \
            -Wl,--out-implib=$(TARGET_DLL_USE) -Wl,--export-all-symbols \
            -Wl,--enable-auto-import \
            -Wl,--whole-archive \
            $(addprefix $(TARGET_DIR),$(OBJECTS)) \
            -Wl,--no-whole-archive

$(TARGET_DLL_DEF): $(TARGET_DLL)
	$(LD) --output-def $@ $(TARGET_DLL) -o $(TARGET_DIR)a.exe
	rm $(TARGET_DIR)a.exe

#%.o: %.c $(MAKEFILE)
%.o: %.c $(DUMMY_DIR_FILE)
	@echo Building $@
	$(CC) $(CFLAGS) -c -o $(TARGET_DIR)$@ $<
	-@rm -f $(@:.o=.d)
	$(CC) -M $(CFLAGS) -c -o $(TARGET_DIR)$(@:.o=.d) $<

#%.o: %.cpp $(MAKEFILE)
%.o: %.cpp $(DUMMY_DIR_FILE)
	@echo Building $@
	$(CC) $(CPPFLAGS) -c -o $(TARGET_DIR)$@ $<
	-@rm -f $(@:.o=.d)
	$(CC) -M $(CPPFLAGS) -c -o $(TARGET_DIR)$(@:.o=.d) $<
#	$(CC) --version

$(DUMMY_DIR_FILE):
	-mkdir $(TARGET_DIR)
	echo Dummy file >$@

$(DUMMY_LIB_DIR_FILE):
	-mkdir $(dir $@)
	echo Dummy file >$@

targetdir:
	-mkdir $(sort $(dir $(OBJECTS)))

##################################################################
# cleaning rule
##################################################################

clean:
	rm -f $(addprefix $(TARGET_DIR), *.o *.d *~ $(TARGET))
	rm -f $(OBJECTS)
	rm -f $(OBJECTS:.o=.d)
	rm -f version.h
	rm -r $(TARGET_DIR)
	rm -r $(LIB_DIR)

dep_test:
	@echo DEPFILES=$(DEPFILES)
	@echo wildcard=$(wildcard $(DEPFILES))

include version.mk
$(TARGET).o: version.h

include $(wildcard $(DEPFILES))
