LIBDIRS= -L/usr/X11R6/lib
LDLIBS = -lglut -lGL -lGLU -lX11 -lm -lpng -std=c++11

CPPFLAGS= -O3 
LDFLAGS= $(CPPFLAGS) $(LIBDIRS)

TARGETS = 

SRCS = 

OBJS =  $(SRCS:.cpp=.o)

CXX = g++

default: $(TARGETS)
