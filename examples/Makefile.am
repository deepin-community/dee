noinst_PROGRAMS = \
 master-model \
 peers \
 slave-model
# synced-lists

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
  $(GCC_FLAGS) \
	$(DEE_CFLAGS) \
	$(MAINTAINER_CFLAGS)

master_model_SOURCES = master-model.c
master_model_LDADD = $(top_builddir)/src/libdee-1.0.la $(DEE_LIBS)

peers_SOURCES = peers.c
peers_LDADD = $(top_builddir)/src/libdee-1.0.la $(DEE_LIBS)

slave_model_SOURCES = slave-model.c
slave_model_LDADD = $(top_builddir)/src/libdee-1.0.la $(DEE_LIBS)

## Comment in to compile synced-lists againt uninstalled dev libs
#synced_lists_SOURCES = synced-lists.c
#synced_lists_LDADD = $(top_builddir)/src/libdee-1.0.la
#synced_lists_LDFLAGS = `pkg-config --libs gtk+-2.0`
#synced_lists_CFLAGS = `pkg-config --cflags gtk+-2.0`

EXTRA_DIST = \
    list-peers \
    master-model.vala \
    master-model.py \
    pythontricks.py \
    slave-model.py
