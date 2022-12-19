ACLOCAL_AMFLAGS = -I build/autotools

include $(top_srcdir)/Makefile.am.coverage

SUBDIRS = build src doc examples tools vapi bindings

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dee-1.0.pc

if HAVE_ICU
pkgconfig_DATA += dee-icu-1.0.pc
endif

if WANT_TESTS
SUBDIRS += tests

# Test framework
.PHONY: check-report full-report check-headless
check-report full-report check-headless:
	$(MAKE) -C tests/ $(@)
endif

CLEANFILES = dee-1.0.pc
DISTCLEANFILES = dee-1.0.pc
EXTRA_DIST = \
	autogen.sh \
	dee-1.0.pc.in \
	dee-icu-1.0.pc.in \
	COPYING.GPL

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --with-pygi-overrides-dir='$$(pyexecdir)'/gi/overrides

benchmark:
	cd tests && make benchmark
