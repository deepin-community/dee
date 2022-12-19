NULL =

vapidir = $(datadir)/vala/vapi

if HAVE_INTROSPECTION
if HAVE_VAPIGEN

dee-1.0.vapi: $(top_builddir)/src/Dee-$(GIR_VERSION).gir $(srcdir)/Dee-1.0-custom.vala $(srcdir)/Dee-1.0.metadata
	$(AM_V_GEN)$(VALA_API_GEN) --pkg gio-2.0 --library dee-1.0 --metadatadir=$(srcdir) $(filter %.gir %.vala,$^)
	@touch $@

endif
endif

dist_vapi_DATA = \
        dee-1.0.deps \
        dee-1.0.vapi \
        $(NULL)

dist_noinst_DATA = \
        Dee-1.0.metadata \
        Dee-1.0-custom.vala \
        $(NULL)

DISTCLEANFILES = dee-1.0.vapi

EXTRA_DIST = \
        $(NULL)
