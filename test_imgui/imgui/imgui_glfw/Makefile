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

EXE = example_glfw_opengl3
SOURCES = main.cpp
SOURCES += ../imgui_impl_glfw.cpp ../imgui_impl_opengl3.cpp
SOURCES += ../../imgui.cpp ../../imgui_demo.cpp ../../imgui_draw.cpp ../../imgui_widgets.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -I../ -I../../
CXXFLAGS += -g -Wall -Wformat
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
		CXXFLAGS += 
		LIBS += -lopengl32 -lgdi32
	else
		CC=$(TOOLS_CC)
		CXX=$(TOOLS_CXX)
		CXXFLAGS += 
		LIBS += -lGL -ldl -lpthread -lrt
	endif	
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
#	ECHO_MESSAGE = "build system Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	CC=x86_64-kgp-mingw32-$(TOOLS_CC).exe
	CXX=x86_64-kgp-mingw32-$(TOOLS_CXX).exe
	LIBS +=-lgdi32 -lopengl32 -limm32
	CXXFLAGS +=
	CFLAGS = $(CXXFLAGS)
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

all: build_info $(EXE)
	@echo Build complete

$(EXE):$(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

build_info:
	@echo build system ... $(UNAME_S)
	@echo host system  ... $(HOST)

clean:
	rm -f $(EXE) $(EXE).exe $(OBJS)
