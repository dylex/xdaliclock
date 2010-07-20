# xdaliclock, Copyright (c) 1991-2002 by Jamie Zawinski.

SHELL		= /bin/sh
TARFILES	= README Makefile version.h xdaliclock.lsm \
		  config.guess config.sub install-sh
NUMBERS		= numbers0/*.xbm numbers1/*.xbm \
		  numbers2/*.xbm numbers3/*.xbm
TAR		= tar
COMPRESS	= gzip --verbose --best
COMPRESS_EXT	= gz


default:
	@echo ''; \
	 echo '        please make one of: "x", "palm", "all", or "clean"'; \
	 echo ''; \
	 exit 5

all: X11 palm

X11::   x11
X::     X11
x::     X11
palm::  Palm
pilot:: Palm
Pilot:: Palm

x11: X11/Makefile
	@echo '==============================================================='
	@echo ''
	cd X11 ; make

Palm: palm/Makefile
	@echo '==============================================================='
	@echo ''
	cd palm ; make

X11/Makefile:
	@echo '==============================================================='
	@echo ''
	cd X11 ; ./configure

palm/Makefile:
	@echo '==============================================================='
	@echo ''
	cd palm ; ./configure --host=m68k-palmos --build=`../config.guess`

clean: X11/Makefile palm/Makefile
	cd X11 ; make $@
	cd palm ; make $@

distclean: X11/Makefile palm/Makefile
	cd X11 ; make $@
	cd palm ; make $@

install:
	@echo ''; \
	 echo '        to install the X version: "cd X11; make install".';   \
	 echo ''; \
	 echo '        to install the Palm version: "cd palm; make", and';   \
	 echo '        then download "daliclock.prc" to your PalmOS device.';\
	 echo ''; \
	 exit 5

tar: X11/Makefile palm/Makefile
	@								    \
  for dir in X11 palm ; do						    \
   ( cd $$dir ;								    \
     rm -f configure ;							    \
     autoconf ;								    \
     ./config.status ;							    \
     $(MAKE) distdepend );						    \
  done ;								    \
  sh xdaliclock.lsm.sh > xdaliclock.lsm.$$$$ ;				    \
  mv xdaliclock.lsm.$$$$ xdaliclock.lsm ;				    \
  (cd X11; make update_spec_version) ;					    \
  (cd palm; make update_rsc_version) ;					    \
  NAME=`sed -n								    \
  's/[^0-9]*\([0-9]\.[0-9][0-9]*\).*/xdaliclock-\1/p' version.h` ;	    \
  rm -rf $$NAME ; ln -s . $$NAME ;					    \
  FILES= ;								    \
  for subdir in X11 palm ; do						    \
    d=`pwd` ;								    \
    cd $$subdir ;							    \
    FILES="$$FILES `$(MAKE) echo_tarfiles				    \
      | grep -v '^.*make\['						    \
      | sed \"s|^|$$subdir/|g;s| | $$subdir/|g\"			    \
      ` ";								    \
    cd $$d ; done ;							    \
  echo creating tar file $${NAME}.tar.$(COMPRESS_EXT)... ;		    \
  $(TAR) -vchf -							    \
    `echo $(TARFILES) $$FILES $(NUMBERS)				    \
   | sed "s|^|$$NAME/|g; s| | $$NAME/|g" `				    \
   | $(COMPRESS) > $${NAME}.tar.$(COMPRESS_EXT) ;			    \
  rm $$NAME ;								    \
  ls -ldF $${NAME}.tar.$(COMPRESS_EXT)

www::
	@								    \
  DEST=$$HOME/www/xdaliclock ;						    \
  VERS=`sed -n 's/[^0-9]*\([0-9]\.[0-9][0-9]*\).*/\1/p' version.h`	 ;  \
  VERS2=`echo $$VERS | sed 's/[.]//g'`					 ;  \
  HEAD="daliclock-$$VERS"						 ;  \
  HEAD2="daliclock-$$VERS2"						 ;  \
  TAR="x$$HEAD.tar.gz"							 ;  \
  PRC="$$HEAD2.prc"							 ;  \
  ZIP="$$HEAD2.zip"							 ;  \
									    \
  if [ ! -f $$TAR ]; then						    \
    echo "$$TAR does not exist!  Did you forget to \`make tar'?" ;	    \
    exit 1 ; 								    \
  fi ;									    \
  chmod a-w $$TAR ;							    \
  if [ -f $$DEST/$$TAR ]; then						    \
    echo -n "WARNING: $$DEST/$$TAR already exists!  Overwrite? ";	    \
    read line;								    \
    if [ "x$$line" != "xyes" -a "x$$line" != "xy" ]; then		    \
      exit 1 ; 								    \
    fi ;								    \
  fi ;									    \
  if [ -f $$DEST/$$PRC ]; then						    \
    echo -n "WARNING: $$DEST/$$PRC already exists!  Overwrite? ";	    \
    read line;								    \
    if [ "x$$line" != "xyes" -a "x$$line" != "xy" ]; then		    \
      exit 1 ; 								    \
    fi ;								    \
  fi ;									    \
  cp -p $$TAR $$DEST/$$TAR ;						    \
  chmod u+w $$DEST/$$TAR ;						    \
  cp -p palm/daliclock.prc $$DEST/$$PRC ;				    \
  cd $$DEST ;								    \
									    \
  rm -f $$ZIP ;								    \
  zip -q9 $$ZIP $$PRC ;							    \
									    \
  TMP=/tmp/xd.$$$$ ;							    \
  sed -e "s/daliclock-[0-9]\.[0-9][0-9]*/$$HEAD/g"			    \
      -e "s/daliclock-[0-9][0-9][0-9]*/$$HEAD2/g" index.html > $$TMP ;	    \
  echo '' ;								    \
  diff -u0 index.html $$TMP ;						    \
  echo '' ;								    \
									    \
  OLDEST=`ls xdaliclock*.tar.gz | head -1` ;				    \
  echo -n "Delete $$DEST/$$OLDEST? ";					    \
  read line;								    \
  if [ "x$$line" = "xyes" -o "x$$line" = "xy" ]; then			    \
    set -x ;								    \
    rm $$OLDEST ;							    \
    cvs remove $$OLDEST ;						    \
    set +x ;								    \
  fi ;									    \
									    \
  OLDEST=`ls daliclock*.prc | head -1` ;				    \
  OLDEST2=`echo $$OLDEST | sed 's/prc/zip/'` ;				    \
  echo -n "Delete $$DEST/$$OLDEST and $$DEST/$$OLDEST2? ";		    \
  read line;								    \
  if [ "x$$line" = "xyes" -o "x$$line" = "xy" ]; then			    \
    set -x ;								    \
    rm $$OLDEST $$OLDEST2 ;						    \
    cvs remove $$OLDEST $$OLDEST2 ;					    \
  else									    \
    set -x ;								    \
  fi ;									    \
									    \
  cvs add -kb $$TAR $$PRC $$ZIP ;					    \
  set +x ;								    \
									    \
  cat $$TMP > index.html ;						    \
  rm -f $$TMP ;								    \
									    \
  cvs commit -m "$$VERS"

