EXTENSION     = pg_sphere
EXT_VERSION   = 1.0
EXT_VERSIONED = $(EXTENSION)--$(EXT_VERSION)
MODULE_big    = $(EXTENSION)
OBJS          = sscan.o sparse.o sbuffer.o vector3d.o point.o \
                euler.o circle.o line.o ellipse.o polygon.o \
                path.o box.o output.o gq_cache.o gist.o key.o crossmatch.o \
		gnomo.o healpix.o
DATA_built    = $(EXT_VERSIONED).sql $(EXTENSION).control
REGRESS_SQL   = tables points euler circle line ellipse poly path box index \
                contains_ops contains_ops_compat bounding_box_gist gnomo healpix
REGRESS       = init $(REGRESS_SQL)
TESTS         = init_test $(REGRESS_SQL)
DOCS          = README.$(EXTENSION) COPYRIGHT.$(EXTENSION)
                # order of sql files is important
EXT_SQL       = pgs_types pgs_point pgs_euler pgs_circle pgs_line pgs_ellipse \
                pgs_polygon pgs_path pgs_box pgs_contains_ops \
                pgs_contains_ops_compat pgs_gist pgs_crossmatch gnomo healpix

EXTRA_CLEAN   = $(EXT_VERSIONED).sql $(EXT_VERSIONED).sql.in \
                $(EXTENSION).test.sql $(EXTENSION).control logfile

CRUSH_TESTS   = init_extended circle_extended 

SHLIB_LINK += -lchealpix
   
ifdef USE_PGXS
  ifndef PG_CONFIG
    PG_CONFIG := pg_config
  endif
  PGXS := $(shell $(PG_CONFIG) --pgxs)
  include $(PGXS)
else
  subdir = contrib/pg_sphere
  top_builddir = ../..
  PG_CONFIG := $(top_builddir)/src/bin/pg_config/pg_config
  include $(top_builddir)/src/Makefile.global
  include $(top_srcdir)/contrib/contrib-global.mk
endif

PGVERSION95PLUS=$(shell $(PG_CONFIG) --version |                   \
                  awk '{ split($$2, a, /[^0-9]+/);                 \
                         if (a[1] > 9 || a[1] == 9 && a[2] >= 5) { \
                                 print "y"; } }')

ifeq ($(PGVERSION95PLUS), y)
        PGS_TMP_DIR = --temp-instance=tmp_check
else
        PGS_TMP_DIR = --temp-install=tmp_check --top-builddir=test_top_build_dir
endif

crushtest: REGRESS += $(CRUSH_TESTS)
crushtest: installcheck

test_extended: TESTS += $(CRUSH_TESTS)
test_extended: test

test: $(EXTENSION).test.sql sql/init_test.sql
	$(pg_regress_installcheck) $(PGS_TMP_DIR) $(REGRESS_OPTS) $(TESTS)

$(EXT_VERSIONED).sql.in: $(addsuffix .sql.in, $(EXT_SQL))
	cat $^ > $@

$(EXT_VERSIONED).sql: $(EXT_VERSIONED).sql.in
	echo "-- prevent sourcing this file inadvertently" > $@
	echo '\\echo Please type "CREATE EXTENSION $(EXTENSION);" to activate' \
				'the $(EXTENSION) extension. \\quit' >> $@
	echo >> $@
	cat $< >> $@

$(EXTENSION).test.sql: $(EXT_VERSIONED).sql.in $(shlib)
	sed 's,MODULE_PATHNAME,$(realpath $(shlib)),g' $< >$@

$(EXTENSION).control: $(EXTENSION).control.in
	echo '# '$(EXTENSION)' extension' > $@
	cat $< >> $@
	echo "default_version = '$(EXT_VERSION)'" >> $@
	echo "module_pathname = '"'$$libdir/'$(EXTENSION)"'" >> $@
	echo "relocatable = true" >> $@

sscan.o : sparse.c

sparse.c: sparse.y
ifdef YACC
	$(YACC) -d $(YFLAGS) -p sphere_yy -o sparse.c $<
else
	@$(missing) bison $< $@
endif

sscan.c : sscan.l
ifdef FLEX
	$(FLEX) $(FLEXFLAGS) -Psphere -o$@ $<
else
	@$(missing) flex $< $@
endif

dist : clean sparse.c sscan.c
	find . -name '*~' -type f -exec rm {} \;
	cd .. && tar  --exclude .git -czf pg_sphere.tar.gz pg_sphere && cd -
