# Note: gstjpcnndir is set in configure

##############################################################################
# TODO: change libgstjpcnn.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstjpcnn.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstjpcnn_la_SOURCES = gstjpcnn.c gstjpcnn.h

# compiler and linker flags used to compile this gstjpcnn, set in configure.ac
libgstjpcnn_la_CFLAGS = $(GST_CFLAGS)
libgstjpcnn_la_LIBADD = $(GST_LIBS)
libgstjpcnn_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -ljpcnn
libgstjpcnn_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstjpcnn.h
