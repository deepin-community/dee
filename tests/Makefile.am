NULL = 

noinst_PROGRAMS = \
  test-dee \
  test-benchmark \
  $(NULL)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-DTESTDIR=\""$(top_builddir)/tests"\" \
	-DDEE_COMPILATION \
	$(GCC_FLAGS) \
	$(DEE_CFLAGS) \
	$(MAINTAINER_CFLAGS)

test_benchmark_SOURCES = \
  test-benchmark.c

test_benchmark_LDADD = $(top_builddir)/src/libdee-1.0.la $(DEE_LIBS) -lm

benchmark: test-benchmark
	./test-benchmark

test_dee_SOURCES = \
  test-analyzer.c \
  test-dee.c \
  test-filter-model.c \
  test-glist-result-set.c \
  test-index.c \
  test-model-column.c \
  test-model-complex-column.c \
  test-model-readers.c \
  test-model-rows.c \
  test-model-signals.c \
  test-model-seqnums.c \
  test-model-tags.c \
  test-resource-manager.c \
  test-serializable.c \
  test-transaction.c \
  test-term-list.c \
  $(top_srcdir)/src/dee-glist-result-set.h \
  $(NULL)

test_dee_LDADD = $(top_builddir)/src/libdee-1.0.la $(DEE_LIBS)

if HAVE_ICU
test_dee_SOURCES += test-icu.c
endif

if HAVE_GTX
test_dee_SOURCES += test-model-interactions.c
test_dee_SOURCES += test-peer-interactions.c
test_dee_SOURCES += test-client-server.c

AM_CPPFLAGS += $(GTX_CFLAGS)
test_dee_LDADD += $(GTX_LIBS)

model_helpers = \
  model-helper-add3rows.c \
  model-helper-append1.c \
  model-helper-change3rows.c \
  model-helper-clear3rows.c \
  model-helper-clear6rows.c \
  model-helper-clone3rows.c \
  model-helper-clone3rows-meta.c \
  model-helper-clear3add5.c \
  model-helper-insert1row.c \
  model-helper-introspect.c \
  model-helper-remove3rows.c \
  model-helper-replace.c \
  model-helper-resync3rows.c \
  model-helper-schemaless.c \
  $(NULL)

peer_helpers = \
  peer-helper-1peer.c \
  $(NULL)

server_helpers = \
  server-helper-client.c

noinst_PROGRAMS += \
  $(model_helpers:.c=) \
  $(peer_helpers:.c=) \
  $(server_helpers:.c=) \
  $(NULL)

model_helper_clone3rows_SOURCES = model-helper-clone3rows.c
model_helper_clone3rows_LDADD = $(test_dee_LDADD)

model_helper_clone3rows_meta_SOURCES = model-helper-clone3rows-meta.c
model_helper_clone3rows_meta_LDADD = $(test_dee_LDADD)

model_helper_add3rows_SOURCES = model-helper-add3rows.c
model_helper_add3rows_LDADD = $(test_dee_LDADD)

model_helper_append1_SOURCES = model-helper-append1.c
model_helper_append1_LDADD = $(test_dee_LDADD)

model_helper_change3rows_SOURCES = model-helper-change3rows.c
model_helper_change3rows_LDADD = $(test_dee_LDADD)

model_helper_remove3rows_SOURCES = model-helper-remove3rows.c
model_helper_remove3rows_LDADD = $(test_dee_LDADD)

model_helper_clear3rows_SOURCES = model-helper-clear3rows.c
model_helper_clear3rows_LDADD = $(test_dee_LDADD)

model_helper_clear6rows_SOURCES = model-helper-clear6rows.c
model_helper_clear6rows_LDADD = $(test_dee_LDADD)

model_helper_clear3add5_SOURCES = model-helper-clear3add5.c
model_helper_clear3add5_LDADD = $(test_dee_LDADD)

model_helper_insert1row_SOURCES = model-helper-insert1row.c
model_helper_insert1row_LDADD = $(test_dee_LDADD)

model_helper_schemaless_SOURCES = model-helper-schemaless.c
model_helper_schemaless_LDADD = $(test_dee_LDADD)

model_helper_introspect_SOURCES = model-helper-introspect.c
model_helper_introspect_LDADD = $(test_dee_LDADD)

model_helper_replace_SOURCES = model-helper-replace.c
model_helper_replace_LDADD = $(test_dee_LDADD)

model_helper_resync3rows_SOURCES = model-helper-resync3rows.c
model_helper_resync3rows_LDADD = $(test_dee_LDADD)

peer_helper_1peer_SOURCES = peer-helper-1peer.c
peer_helper_1peer_LDADD = $(test_dee_LDADD)

server_helper_client_SOURCES = server-helper-client.c
server_helper_client_LDADD = $(test_dee_LDADD)

endif # HAVE_GTX

#
# Python tests disabled because of https://bugzilla.gnome.org/show_bug.cgi?id=660647
#
#PYTHON_TESTS_ENV_VARS= \
#  PYTHONPATH=$(top_builddir)/tests:$${PYTHONPATH:+:$$PYTHONPATH} \
#  GI_TYPELIB_PATH=$(top_builddir)/src:$$GI_TYPELIB_PATH \
#  XDG_DATA_DIRS=$(top_builddir)/src:$XDG_DATA_DIRS:/usr/share \
#  LD_LIBRARY_PATH=$(top_builddir)/src/.libs:$$LD_LIBRARY_PATH \
#  DEE_TEST_PYGOBJECT_OVERRIDEDIR=$(top_srcdir)/bindings/python
#
#test-python:
#	$(PYTHON_TESTS_ENV_VARS) python test-python.py

.PHONY: test
test:
	@dbus-test-runner -m 60 --task gtester \
		--parameter --verbose \
		--parameter -o=test-dee-results.xml \
		--parameter -k \
		--parameter ./test-dee
	

.PHONY: check-report full-report
check-report:
	@dbus-test-runner -m 60 --task gtester \
			--parameter -o=test-dee-results.xml \
			--parameter -k \
			--parameter ./test-dee \
		&& ( gtester-report test-dee-results.xml \
				| sed 's/GTester Unit Test Report</>GTester Unit Test Report (normal)</' \
				> test-dee-results.html ) \
		&& gnome-open ./test-dee-results.html

full-report:
	@dbus-test-runner -m 60 --task gtester \
		--parameter -o=test-dee-results.xml \
		--parameter -k \
		--parameter -m=slow \
		--parameter ./test-dee \
	  && ( gtester-report test-dee-results.xml \
	      | sed 's/>GTester Unit Test Report</>GTester Unit Test Report (normal)</' \
	      > test-dee-results.html ) \
	  && ( xdg-open test-dee-results.html )

#run make test as part of make check
#check-local: test test-python
check-local: test

clean-generic:
	rm -rf test-dee-results.xml test-dee-results.html dee-test-resource-manager

