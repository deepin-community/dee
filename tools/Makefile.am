bin_PROGRAMS = \
  dee-tool

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(GCC_FLAGS) \
	$(DEE_CFLAGS) \
	$(MAINTAINER_CFLAGS)

dee_tool_SOURCES = dee-tool.c
dee_tool_LDADD = $(top_builddir)/src/libdee-1.0.la $(DEE_LIBS)
