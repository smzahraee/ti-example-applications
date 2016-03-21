bin_PROGRAMS = pvrscope mpuload

mpuload_CFLAGS = \
	-O0 -g --static 

mpuload_SOURCES = mpuload.c


pvrscope_CXXFLAGS = -I. -lrt -lm -lpthread -ldl
pvrscope_SOURCES = pvrscope.cpp
LIBS = libPVRScopeDeveloper.a
