SUBDIRS = counting pthread_counting pthread_misc file_counting io_bench pipe_counting
MOSTLYCLEANFILES = context.*
EXTRA_DIST = license.txt README
all-local:
	if test -d $(top_builddir)/libcr32/examples; then $(MAKE) $(AM_MAKEFLAGS) -C $(top_builddir)/libcr32/examples all; fi
