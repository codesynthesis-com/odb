# file      : makefile
# license   : GNU GPL; see accompanying LICENSE file

include $(dir $(lastword $(MAKEFILE_LIST)))build/bootstrap.make

all_dirs := libcommon common evolution mysql sqlite pgsql oracle mssql boost qt
dirs := common boost qt

# Evolution and database-specific tests are not run in the multi-database
# configuration.
#
ifneq ($(db_id),common)
dirs += evolution $(db_id)
endif

$(default): $(addprefix $(out_base)/,$(addsuffix /,$(dirs)))

$(dist): data_dist := GPLv2 LICENSE README INSTALL version test.bat \
tester.bat mysql-driver.bat mysql.options sqlite-driver.bat \
sqlite.options pgsql-driver.bat pgsql.options oracle.options \
oracle-driver.bat mssql.options mssql-driver.bat
$(dist): exec_dist := bootstrap tester.in
$(dist): export extra_dist := $(data_dist) $(exec_dist) build.bat
$(dist): export version = $(shell cat $(src_root)/version)

$(dist): $(addprefix $(out_base)/,$(addsuffix /.dist,$(all_dirs)))
	$(call dist-data,$(data_dist))
	$(call dist-exec,$(exec_dist))
	$(call dist-dir,m4)
	$(call meta-vctest,libcommon/libcommon-mysql-vc10.sln,build.bat)
	$(call meta-automake)
	$(call meta-autoconf)

$(test): $(addprefix $(out_base)/,$(addsuffix /.test,$(dirs)))

ifeq ($(db_id),common)
$(foreach d,$(databases),$(eval $(call db-test-dir,$d,$(dirs))))
endif

$(clean): $(addprefix $(out_base)/,$(addsuffix /.clean,$(all_dirs)))

$(call include,$(bld_root)/dist.make)
$(call include,$(bld_root)/meta/vctest.make)
$(call include,$(bld_root)/meta/automake.make)
$(call include,$(bld_root)/meta/autoconf.make)

ifneq ($(filter $(MAKECMDGOALS),dist clean),)
$(foreach d,$(all_dirs),$(call import,$(src_base)/$d/makefile))
else
$(foreach d,$(dirs),$(call import,$(src_base)/$d/makefile))
endif
