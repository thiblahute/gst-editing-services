include $(top_srcdir)/common/check.mak

CHECK_REGISTRY = $(top_builddir)/tests/check/test-registry.xml

plugindir = $(libdir)/gstreamer-@GST_API_VERSION@

REGISTRY_ENVIRONMENT = \
	GST_REGISTRY=$(CHECK_REGISTRY)
# Note: GST_PLUGINS_GOOD_DIR is only set in an uninstalled setup
TESTS_ENVIRONMENT = \
        CK_DEFAULT_TIMEOUT=120                                   \
	$(REGISTRY_ENVIRONMENT)					\
	GST_PLUGIN_SYSTEM_PATH=					\
	GST_PLUGIN_PATH=$(top_builddir)/gnl:$(GST_PLUGINS_GOOD_DIR):$(GST_PLUGINS_BASE_DIR):$(GST_PLUGINS_DIR)

# the core dumps of some machines have PIDs appended
CLEANFILES = core.* test-registry.*

SUPPRESSIONS = $(top_srcdir)/common/gst.supp # $(srcdir)/gst-plugins-bad.supp

clean-local: clean-local-check

check_PROGRAMS = \
	ges/backgroundsource\
	ges/basic	\
	ges/layer	\
	ges/effects	\
	ges/filesource	\
	ges/simplelayer	\
	ges/timelineobject	\
	ges/timelineedition	\
	ges/titles\
	ges/transition	\
	ges/overlays\
	ges/text_properties\
	ges/save_and_load \
	gnl/simple	\
	gnl/complex	\
	gnl/gnlsource	\
	gnl/gnloperation	\
	gnl/gnlcomposition	\
	gnl/seek

noinst_HEADERS = \
	gnl/common.h

TESTS = $(check_PROGRAMS)

AM_CFLAGS = -DGST_USE_UNSTABLE_API  -I$(top_srcdir) $(GST_PLUGINS_BASE_CFLAGS) $(GST_OBJ_CFLAGS) \
	$(GST_CHECK_CFLAGS) $(GST_OPTION_CFLAGS) $(GST_CFLAGS) \
	-UG_DISABLE_ASSERT -UG_DISABLE_CAST_CHECKS

LDADD = $(top_builddir)/ges/libges-@GST_API_VERSION@.la \
	$(GST_PLUGINS_BASE_LIBS) -lgstpbutils-$(GST_API_VERSION) \
	$(GST_OBJ_LIBS) $(GST_CHECK_LIBS)

EXTRA_DIST = #gst-plugins-bad.supp

SUPPRESSIONS = $(top_srcdir)/common/gst.supp
