plugin_LTLIBRARIES = libgstges.la

libgstges_la_SOURCES = gstges.c gstgessource.c
libgstges_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) \
	-I$(top_srcdir)

libgstges_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) \
	$(GST_BASE_LIBS) $(GST_LIBS) \
	-lges-$(GST_API_VERSION)

libgstges_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstges_la_LIBTOOLFLAGS = $(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS = gstgessource.h

