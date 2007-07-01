all: package

RPM    = MediaCaster-Server-1.50Beta-1dec.src.rpm
MACROS = .rpmmacros


copy: ${RPM}
	cp SRPMS/$< //dec/tmp


package: ${RPM}


clean:
	rm -fr BUILD SOURCES SPECS ${MACROS}


clobber: clean
	rm -f ${RPM}
	rm -fr RPMS SRPMS


${RPM}: SOURCES/sources.tar mcaster.spec
	HOME=. rpmbuild -bs mcaster.spec


SOURCES/sources.tar: ROOT/* stage
	( cd ROOT; tar -cf ../$@ * )



stage: BUILD RPMS SRPMS SOURCES SPECS ${MACROS}


${MACROS}:
	@echo "%_topdir " `pwd` > $@


BUILD:
	mkdir $@


RPMS:
	mkdir $@


SRPMS:
	mkdir $@


SOURCES:
	mkdir $@


SPECS:
	mkdir $@
