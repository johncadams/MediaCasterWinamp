VERSION   = 1.60BETA
BUILD_DIR = Build
# RPM_DIR   = $(BUILD_DIR)/RPM
RPM_DIR   = "/cygdrive/c/cygwin2/usr/src/rpm"


all:    mkdirs $(BUILD_DIR)/MediaCaster.rpm
mkdirs:	$(RPM_DIR)/BUILD $(RPM_DIR)/SOURCES $(RPM_DIR)/SPECS $(RPM_DIR)/RPMS $(RPM_DIR)/SRPMS


$(BUILD_DIR)/MediaCaster.rpm: $(RPM_DIR)/SPECS/mcaster.spec mkdirs
	rm -f $(BUILD_DIR)/*.rpm
	@echo 'Creating server installer: $@'
	(cd $(RPM_DIR); HOME=. rpmbuild -bb SPECS/mcaster.spec)
	mv $(RPM_DIR)/RPMS/*/*rpm $(BUILD_DIR)


$(RPM_DIR)/SPECS/mcaster.spec: $(RPM_DIR)/SPECS Apache/mcaster.spec
	cp Apache/mcaster.spec $@


$(RPM_DIR)/BUILD:
	mkdir -p $@/usr/MCaster
	cp -r Apache/* $(RPM_DIR)/BUILD/usr/MCaster
	find $(RPM_DIR)/BUILD -name CVS | xargs rm -fr


$(RPM_DIR)/SOURCES: $(RPM_DIR)/BUILD
	mkdir -p $@
	(cd $(RPM_DIR)/BUILD; tar -cvf ../SOURCES/sources.tar .)

	
$(RPM_DIR)/SPECS:
	mkdir -p $@

	
$(RPM_DIR)/RPMS:
	mkdir -p $@

	
$(RPM_DIR)/SRPMS:
	mkdir -p $@
