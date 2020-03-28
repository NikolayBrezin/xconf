#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

#CXX = g++
#CXX = clang++

TOOLS_CC=gcc
TOOLS_CXX=g++
TOOLS_STRIP=strip

APP_NAME=example_glfw_opengl3


SOURCES = main.cpp
SOURCES += ../imgui_impl_glfw.cpp ../imgui_impl_opengl3.cpp
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp ../../imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

LTO ?= no

CXXFLAGS = -I../ -I../../
CXXFLAGS += -g3 -ggdb -Wall -Wformat -ffunction-sections -fdata-sections

BUILD_DATE_STR="\"$(shell date +%Y%m%d:%H:%M:%S)"\"
BUILD_DATE=$(shell date +%Y%m%d)
BUILD_TIME=$(shell date +%H%M%S)

LDFLAGS=-Wl,-gc-sections -Wl,--defsym,__firmware_build_date__=$(BUILD_DATE),--defsym,__firmware_build_time__=$(BUILD_TIME)

#optimisation
CXXFLAGS+= -pipe -Ofast
<<<<<<< Updated upstream
<<<<<<< Updated upstream
ifeq ($(LTO),yes)
=======
ifeq ($(HOST),mingw)
>>>>>>> Stashed changes
=======
ifeq ($(HOST),mingw)
>>>>>>> Stashed changes
	CXXFLAGS+=-flto=8
else
	CXXFLAGS+=-fno-lto
endif

LIBS =-lglfw3

##---------------------------------------------------------------------
## OPENGL LOADER
##---------------------------------------------------------------------

## Using OpenGL loader: gl3w [default]
SOURCES += ../libs/gl3w/GL/gl3w.c
CXXFLAGS += -I../libs/gl3w -DIMGUI_IMPL_OPENGL_LOADER_GL3W

## Using OpenGL loader: glew
## (This assumes a system-wide installation)
# CXXFLAGS += -lGLEW -DIMGUI_IMPL_OPENGL_LOADER_GLEW

## Using OpenGL loader: glad
# SOURCES += ../libs/glad/src/glad.c
# CXXFLAGS += -I../libs/glad/include -DIMGUI_IMPL_OPENGL_LOADER_GLAD

## Using OpenGL loader: glbinding
## This assumes a system-wide installation
## of either version 3.0.0 (or newer)
# CXXFLAGS += -lglbinding -DIMGUI_IMPL_OPENGL_LOADER_GLBINDING3
## or the older version 2.x
# CXXFLAGS += -lglbinding -DIMGUI_IMPL_OPENGL_LOADER_GLBINDING2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

#TODO (1) доделать `pkg-config --cflags glfw3` добавлением 'правильных' glfw3.pc в BUILD/HOST/lib

# хост система по умолчанию - mingw
HOST ?= mingw

ifeq ($(UNAME_S), Linux) #LINUX
	ifeq ($(HOST),mingw)
		CC=x86_64-kgp-mingw32-$(TOOLS_CC)
		CXX=x86_64-kgp-mingw32-$(TOOLS_CXX)
		LD=$(CXX)
		STRIP=$(TOOLS_STRIP)
		CXXFLAGS += 
		LIBS += -lopengl32 -lgdi32
		BIN=$(APP_NAME)
	else
		CC=$(TOOLS_CC)
		CXX=$(TOOLS_CXX)
		LD=$(CXX)
		STRIP=$(TOOLS_STRIP)
		CXXFLAGS += 
		LIBS += -lGL -ldl -lpthread -lrt
		BIN=$(APP_NAME).exe
	endif	
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	CC=x86_64-kgp-mingw32-$(TOOLS_CC).exe
	CXX=x86_64-kgp-mingw32-$(TOOLS_CXX).exe
	STRIP=$(TOOLS_STRIP).exe
	LD=$(CXX)
	LIBS +=-lgdi32 -lopengl32 -limm32
	CXXFLAGS +=
	CFLAGS = $(CXXFLAGS)
	BIN=$(APP_NAME).exe
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------



%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../../%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:../libs/gl3w/GL/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:../libs/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: build_info $(BIN)
	@echo Build complete

$(BIN):$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(CXXFLAGS) $(LIBS)

build_info:
	@echo build system ... $(UNAME_S)
	@echo host system  ... $(HOST)

clean:
	rm -f $(BIN) $(OBJS)

strip:
	$(STRIP) $(BIN)
