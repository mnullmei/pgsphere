MODULE_big = pg_sphere
OBJS       = sscan.o sparse.o sbuffer.o vector3d.o point.o \
             euler.o circle.o line.o ellipse.o polygon.o \
             path.o box.o output.o gq_cache.o gist.o key.o \
             gnomo.o healpix.o

EXTENSION   = pg_sphere
DATA_built  = pg_sphere--1.1.5beta0gavo.sql \
			  pg_sphere--unpackaged--1.1.5beta0gavo.sql \
			  pg_sphere--unpackaged_gavo--1.1.5beta0gavo.sql \
			  pg_sphere--1.0--1.0_gavo.sql \
			  pg_sphere--1.0_gavo--1.1.5beta0gavo.sql

DOCS        = README.pg_sphere COPYRIGHT.pg_sphere
REGRESS     = init tables points euler circle line ellipse poly path box index \
			  contains_ops contains_ops_compat bounding_box_gist gnomo healpix
REGRESS_9_5 = index_9.5 # experimental for spoint3

SHLIB_LINK += -lchealpix

EXTRA_CLEAN = $(PGS_SQL)

CRUSH_TESTS = init_extended circle_extended 

# order of sql files is important
PGS_SQL     = pgs_types.sql pgs_point.sql pgs_euler.sql pgs_circle.sql \
   pgs_line.sql pgs_ellipse.sql pgs_polygon.sql pgs_path.sql \
   pgs_box.sql pgs_contains_ops.sql pgs_contains_ops_compat.sql \
   pgs_gist.sql gnomo.sql pgs_gist_pointkey.sql \
   healpix.sql pgs_gist_spoint3.sql
PGS_SQL_9_5 = pgs_9.5.sql # experimental for spoint3

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

# experimental for spoint3
pg_version := $(word 2,$(shell $(PG_CONFIG) --version))
pg_version_9_5_plus = $(if $(filter-out 9.1% 9.2% 9.3% 9.4%,$(pg_version)),y,n)
#
ifeq ($(pg_version_9_5_plus),y)
	REGRESS += $(REGRESS_9_5)
	PGS_SQL += $(PGS_SQL_9_5)
endif

crushtest: REGRESS += $(CRUSH_TESTS)
crushtest: installcheck

pg_sphere--1.1.5beta0gavo.sql: $(addsuffix .in, $(PGS_SQL))
	cat $^ > $@

# for "create extension from unpacked*":

UPGRADE_UNP_COMMON =  pgs_types.sql pgs_point.sql pgs_euler.sql pgs_circle.sql \
	pgs_line.sql pgs_ellipse.sql pgs_polygon.sql pgs_path.sql \
    pgs_box.sql pgs_contains_ops_compat.sql pgs_gist.sql \
	pgs_gist_contains_ops.sql contains-ops-fixes-1.sql

AUGMENT_UNP_COMMON = pgs_contains_ops.sql gnomo.sql

# for vanilla 1.1.1 users
AUGMENT_UNP_111 = $(AUGMENT_UNP_COMMON) pgs_gist_pointkey.sql

# for 1.1.2+ users: 'from unpacked_1.1.2plus'
AUGMENT_UNP_FOR_112plus = $(AUGMENT_UNP_COMMON)
UPGRADE_UNP_FOR_112plus = pgs_gist_pointkey.sql pgs_gist_drop_spoint2.sql.in

# for "alter extension":

# TODO: add dynamic pl/pgsql to do perform an additional
#    "ALTER EXTENSION pg_sphere UPDATE TO '1.1.5_from_before_2016-02-07';"
# if required.
#
# default 1.0 -> 1.1.5
UPGRADE_1_0_PRE_xxxxxx = contains-ops-fixes-2.sql
# '1.1.5_from_2015-08-31'
UPGRADE_1_0_PRE_AAF2D5 = contains-ops-fixes-1.sql pgs_gist_drop_spoint2.sql.in \
						pgs_gist_contains_ops.sql pgs_contains_ops.sql gnomo.sql

# vanilla 'create from unpackaged' must assume 1.1.1
# ...

# create "create extension from unpacked*" files

# create "alter extension" files


ifeq ($(pg_version_9_5_plus),y)
# 1.1.1.5 -> 1.1.5.1 for Postgres 9.5+ features
else
endif

# local stuff follows here, next will be "beta2"

AUGMENT_GAVO_111 = $(AUGMENT_UNP_111) healpix.sql # for vanilla 1.1.1 users
UPGRADE_GAVO_111 = $(UPGRADE_UNP_COMMON)

# add new Healpix functions and experimental spoint3
AUGMENT_FROM_GAVO = healpix.sql pgs_gist_spoint3.sql

AUGMENT_UNP_115B0G = $(AUGMENT_FROM_GAVO)
UPGRADE_UNP_115B0G = $(UPGRADE_UNP_COMMON) gnomo.sql healpix_old.sql

AUGMENT_1_0_115B0G = $(AUGMENT_FROM_GAVO)
UPGRADE_1_0_115B0G = contains-ops-fixes-2.sql pgs_gist_drop_spoint2.sql

# test installation 0
pg_sphere--unpackaged--1.1.5beta0gavo.sql: $(addsuffix .in, \
		$(AUGMENT_GAVO_111) \
		$(addprefix upgrade_scripts/, $(UPGRADE_GAVO_111)))
	cat upgrade_scripts/$@.in $^ > $@

# test installation A
pg_sphere--unpackaged_gavo--1.1.5beta0gavo.sql: $(addsuffix .in, \
		$(AUGMENT_UNP_115B0G) \
		$(addprefix upgrade_scripts/, $(UPGRADE_UNP_115B0G)))
	cat upgrade_scripts/$@.in $^ > $@

# test installation B
pg_sphere--1.0--1.0_gavo.sql: # dummy upgrade to allow for descriptive names
	cat upgrade_scripts/$@.in > $@
pg_sphere--1.0_gavo--1.1.5beta0gavo.sql: $(addsuffix .in, \
		$(AUGMENT_1_0_115B0G) \
		$(addprefix upgrade_scripts/, $(UPGRADE_1_0_115B0G)))
	cat upgrade_scripts/$@.in $^ > $@

# end of local stuff

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
	cd .. && tar  --exclude CVS -czf pg_sphere.tar.gz pg_sphere && cd -
