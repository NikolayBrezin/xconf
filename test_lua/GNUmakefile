TOOLS_CC=gcc
TOOLS_CXX=g++
TOOLS_STRIP=strip
TOOLS_AR=ar
TOOLS_RANLIB=ranlib
UNAME_S := $(shell uname -s)
BUILD_DATE_STR="\"$(shell date +%Y%m%d:%H:%M:%S)"\"
BUILD_DATE=$(shell date +%Y%m%d)
BUILD_TIME=$(shell date +%H%M%S)





APP_NAME= lua_test
APP_LIBS= 



LTO ?= no
COMPILE_FLAGS = -g3 -ggdb -Wall -Wformat -ffunction-sections -fdata-sections -Wp,-M,-MP,-MT,$@,-MF,.dep/$(@F).dep



LDFLAGS=-Wl,-gc-sections -Wl,--defsym,__firmware_build_date__=$(BUILD_DATE),--defsym,__firmware_build_time__=$(BUILD_TIME)

#optimisation
CXXFLAGS+= -pipe -Ofast
ifeq ($(LTO),yes)
	FLAGS+=-flto=8
else
	FLAGS+=-fno-lto
endif

CFLAGS= -std=gnu2x $(COMPILE_FLAGS)
CXXFLAGS= -std=gnu++2a $(COMPILE_FLAGS)  -I. -Ijson -Igl3w -Iimgui  -DIMGUI_IMPL_OPENGL_LOADER_GL3W 

LIBS = -llua



#TODO (1) доделать `pkg-config --cflags glfw3` добавлением 'правильных' glfw3.pc в BUILD/HOST/lib

# хост система по умолчанию - mingw
HOST ?= mingw

ifeq ($(UNAME_S), Linux) #LINUX
	ifeq ($(HOST),mingw)
		CC=x86_64-kgp-mingw32-$(TOOLS_CC)
		CXX=x86_64-kgp-mingw32-$(TOOLS_CXX)
		LD=$(CXX)
		STRIP=x86_64-kgp-mingw32-$(TOOLS_STRIP)
		AR=x86_64-kgp-mingw32-$(TOOLS_AR)
		RANLIB=x86_64-kgp-mingw32-$(TOOLS_RANLIB)
		LIBS += -lopengl32 -lgdi32 -lsetupapi
		BIN=$(APP_NAME).exe
	else
		CC=$(TOOLS_CC)
		CXX=$(TOOLS_CXX)
		LD=$(CXX)
		STRIP=$(TOOLS_STRIP)
		AR=$(TOOLS_AR)
		RANLIB=$(TOOLS_RANLIB)
		LIBS += -lGL -ldl -lpthread -lrt
		BIN=$(APP_NAME)
	endif	
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	CC=$(TOOLS_CC).exe
	CXX=$(TOOLS_CXX).exe
	STRIP=$(TOOLS_STRIP).exe
	AR=$(TOOLS_AR).exe
	RANLIB=$(TOOLS_RANLIB).exe
	STRIP=$(TOOLS_STRIP).exe
	LD=$(CXX)
	LIBS +=-lgdi32 -lopengl32 -limm32 -lsetupapi
	BIN=$(APP_NAME).exe
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------
all: $(BIN)

build_info:
	@echo build system ... $(UNAME_S)
	@echo host system  ... $(HOST)

clean:
	rm -fr $(BIN) .dep

strip:
	$(STRIP) $(BIN)


%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


# ------------------------- lua_test -------------------------
APP_SRCS=	lua_test.cpp 
APP_OBJ = $(addsuffix .o, $(basename $(APP_SRCS)))

$(BIN):$(APP_LIBS) $(APP_OBJ)
	$(LD) $(LDFLAGS) -o $@ $(APP_OBJ) $(CXXFLAGS) $(APP_LIBS) $(LIBS)
#	$(STRIP) $@
#--------------------------------------------------------------


-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

