AUTOMAKE_OPTIONS = nostdinc

CPPFLAGS_COMMON = \
	-I$(abs_builddir) \
	-I$(abs_builddir)/../lib \
	-I$(abs_srcdir) \
	-I$(abs_srcdir)/../lib \
	-I$(includedir)

if ENABLE_STATIC_MODULE

##################################################
# STATIC MODULES BUILT INTO MAIN LIBRARY
##################################################
LDFLAGS_COMMON =-L$(libdir)
LIBADD_COMMON =

noinst_LTLIBRARIES =

if ENABLE_MOD_CON
noinst_LTLIBRARIES += libmoo-con.la
endif

if ENABLE_MOD_FFI
noinst_LTLIBRARIES += libmoo-ffi.la
endif

noinst_LTLIBRARIES += libmoo-io.la

if ENABLE_MOD_SCK
noinst_LTLIBRARIES += libmoo-sck.la
endif

noinst_LTLIBRARIES += libmoo-stdio.la

if ENABLE_MOD_X11
noinst_LTLIBRARIES += libmoo-x11.la
endif

else

##################################################
# DYNAMIC MODULES 
##################################################
LDFLAGS_COMMON = -L$(abs_builddir)/../lib -L$(libdir) -version-info 1:0:0 -no-undefined
LIBADD_COMMON = ../lib/libmoo.la

pkgmodexecdir = $(libdir)
pkgmodexec_LTLIBRARIES =

if ENABLE_MOD_CON
pkgmodexec_LTLIBRARIES += libmoo-con.la 
endif

if ENABLE_MOD_FFI
pkgmodexec_LTLIBRARIES += libmoo-ffi.la
endif

pkgmodexec_LTLIBRARIES += libmoo-io.la

if ENABLE_MOD_SCK
pkgmodexec_LTLIBRARIES += libmoo-sck.la
endif

pkgmodexec_LTLIBRARIES += libmoo-stdio.la

if ENABLE_MOD_X11
pkgmodexec_LTLIBRARIES += libmoo-x11.la
endif

endif

if ENABLE_MOD_CON
libmoo_con_la_SOURCES = _con.c _con.h
libmoo_con_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_con_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_con_la_LIBADD = $(LIBADD_COMMON) $(TERMINAL_LIBS)
endif

if ENABLE_MOD_FFI
libmoo_ffi_la_SOURCES = ffi.c _ffi.h
libmoo_ffi_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_ffi_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_ffi_la_LIBADD = $(LIBADD_COMMON) $(DYNCALL_LIBS) $(FFI_LIBS)
endif

libmoo_io_la_SOURCES = io.c io-file.c _io.h
libmoo_io_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_io_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_io_la_LIBADD = $(LIBADD_COMMON)

if ENABLE_MOD_SCK
libmoo_sck_la_SOURCES = sck.c sck-addr.c _sck.h
libmoo_sck_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_sck_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_sck_la_LIBADD = $(LIBADD_COMMON) $(SOCKET_LIBS)
endif

libmoo_stdio_la_SOURCES = stdio.c _stdio.h
libmoo_stdio_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_stdio_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_stdio_la_LIBADD = $(LIBADD_COMMON)

if ENABLE_MOD_X11
libmoo_x11_la_SOURCES = x11.c _x11.h
libmoo_x11_la_CPPFLAGS = $(CPPFLAGS_COMMON)
libmoo_x11_la_LDFLAGS = $(LDFLAGS_COMMON)
libmoo_x11_la_LIBADD = $(LIBADD_COMMON) -lX11
endif
