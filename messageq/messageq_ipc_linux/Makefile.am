AM_CFLAGS = @DRM_CFLAGS@ 

bin_PROGRAMS = MessageQZeroCpy

MessageQZeroCpy_SOURCES = MessageQZeroCpy.c

MessageQZeroCpy_LDADD = \
	@DRM_LIBS@ @GTHREAD_LIBS@ -ltiipc -ltiipcutils -lmmrpc  
