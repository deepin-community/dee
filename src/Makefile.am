NULL = 
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all

-include $(INTROSPECTION_MAKEFILE)

##
# DBus introspection XML
##
# create a .h file containing the introspection data in a variable for gdbus
%-xml.h : dbus/%.xml
	$(AM_V_GEN)name=`basename $< | sed -e 's/[-\.]/_/g' -e 's/.xml/_xml/g'`; \
	echo "static const gchar $$name[] = " > $@; \
	cat $< | tr \" \' | sed 's/^/\"/g' | sed 's/$$/\"/g' >> $@; \
	echo ";" >> $@;

BUILT_SOURCES += \
    com.canonical.Dee.Model-xml.h \
    com.canonical.Dee.Peer-xml.h

EXTRA_DIST += \
    dbus/com.canonical.Dee.Model.xml \
    dbus/com.canonical.Dee.Peer.xml


##
# Development headers
##
devel_headersdir = $(includedir)/dee-1.0

devel_headers = \
  dee.h \
  dee-analyzer.h \
  dee-file-resource-manager.h \
  dee-filter-model.h \
  dee-filter.h \
  dee-hash-index.h \
  dee-index.h \
  dee-model.h \
  dee-model-reader.h \
  dee-peer.h \
  dee-server.h \
  dee-client.h \
  dee-proxy-model.h \
  dee-resource-manager.h \
  dee-result-set.h \
  dee-sequence-model.h \
  dee-serializable.h \
  dee-serializable-model.h \
  dee-shared-model.h \
  dee-term-list.h \
  dee-text-analyzer.h \
  dee-transaction.h \
  dee-tree-index.h \
  $(NULL)
  

devel_headers_HEADERS = \
  $(devel_headers) \
  $(NULL)

##
# Build libdee
##
lib_LTLIBRARIES = libdee-1.0.la

libdee_1_0_la_SOURCES = \
  $(devel_headers) \
  dee-analyzer.c \
  dee-file-resource-manager.c \
  dee-filter-model.c \
  dee-filter.c \
  dee-glist-result-set.h \
  dee-glist-result-set.c \
  dee-hash-index.c \
  dee-index.c \
  dee-model.c \
  dee-model-reader.c \
  dee-peer.c \
  dee-server.c \
  dee-client.c \
  dee-proxy-model.c \
  dee-resource-manager.c \
  dee-result-set.c \
  dee-sequence-model.c \
  dee-serializable.c \
  dee-serializable-model.c \
  dee-shared-model.c \
  dee-term-list.c \
  dee-text-analyzer.c \
  dee-transaction.c \
  dee-tree-index.c \
  trace-log.h \
  $(BUILT_SOURCES) \
  $(NULL)

libdee_1_0_la_LIBADD = \
  $(DEE_LIBS)

libdee_1_0_la_LDFLAGS = \
  $(DEE_LT_LDFLAGS) \
  $(COVERAGE_LDFLAGS)

libdee_1_0_la_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DG_LOG_DOMAIN=\"dee\" \
	-DPREFIX=\""$(prefix)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DG_DISABLE_DEPRECATED \
	-DDEE_COMPILATION \
	$(GCC_FLAGS) \
	$(DEE_CFLAGS) \
	$(MAINTAINER_CFLAGS) \
	$(COVERAGE_CFLAGS)

if HAVE_ICU
devel_headers += dee-icu.h
libdee_1_0_la_SOURCES += dee-icu-term-filter.c
libdee_1_0_la_LIBADD += $(ICU_LIBS)
endif

##
# If trace logging is not enabled
# all the macros are removed by the preprocessor
##
if ENABLE_TRACE_LOG
libdee_1_0_la_SOURCES += trace-log.c
endif

##
# Signal Marshallers
##
dee-marshal.h: $(srcdir)/dee-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header \
		--prefix=_dee_marshal $(srcdir)/dee-marshal.list \
	 	> dee-marshal.h

dee-marshal.c: $(srcdir)/dee-marshal.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --body \
		--prefix=_dee_marshal $(srcdir)/dee-marshal.list \
		> dee-marshal.c

BUILT_SOURCES += \
	dee-marshal.c \
	dee-marshal.h

EXTRA_DIST += dee-marshal.list

CLEANFILES += \
    $(BUILT_SOURCES)

##
# GObject Introspection
##

if HAVE_INTROSPECTION
irscanner_sources = $(libdee_1_0_la_SOURCES)

dee_gir = Dee-$(GIR_VERSION).gir

Dee-1.0.gir: $(lib_LTLIBRARIES) Makefile
Dee_1_0_gir_SCANNERFLAGS = --c-include="dee.h"
Dee_1_0_gir_INCLUDES = GObject-2.0 GLib-2.0 Gio-2.0
Dee_1_0_gir_LIBS = $(lib_LTLIBRARIES)
Dee_1_0_gir_FILES = $(irscanner_sources)
Dee_1_0_gir_CFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DDEE_COMPILATION \
	$(NULL)
Dee_1_0_gir_EXPORT_PACKAGES = dee-1.0
INTROSPECTION_GIRS += $(dee_gir)

# INTROSPECTION_GIRDIR/INTROSPECTION_TYPELIBDIR aren't the right place to
# install anything - we need to install inside our prefix.
girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)

endif
