## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = timemachine

timemachine_SOURCES = \
	main.c main.h \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	gtkmeter.c gtkmeter.h \
	gtkmeterscale.c gtkmeterscale.h \
	meters.c meters.h \
	threads.c threads.h 

timemachine_LDADD = @PACKAGE_LIBS@
