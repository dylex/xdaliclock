# xdaliclock, Copyright (c) 1991-2006 by Jamie Zawinski.

SHELL		= /bin/sh
TARFILES	= README Makefile version.h \
		  config.guess config.sub install-sh \
		  mac128/README mac128/*.*
NUMBERS		= font/dalifont*.{gif,png,ai,psd.gz} font/*.xbm
TAR		= tar


default:
	@echo ''; \
	 echo '        please make one of: "x", "palm", "webos", "all", or "clean"'; \
	 echo ''; \
	 exit 5

all:: OSX webos palm X11

X11::   x11
X::     X11
x::     X11
gtk::   GTK
OSX::   osx
palm::  Palm
webos:: WebOS

x11:: X11/Makefile
	@echo '==============================================================='
	@echo ''
	cd X11 ; make

GTK:: gtk/Makefile
	@echo '==============================================================='
	@echo ''
	cd gtk ; make

osx::
	@echo '==============================================================='
	@echo ''
	cd OSX ; make

Palm:: palm/Makefile
	@echo '==============================================================='
	@echo ''
	cd palm ; make

WebOS:: webos/Makefile
	@echo '==============================================================='
	@echo ''
	cd webos ; make

X11/Makefile:
	@echo '==============================================================='
	@echo ''
	cd X11 ; ./configure

gtk/Makefile:
	@echo '==============================================================='
	@echo ''
	cd gtk ; ./configure

palm/Makefile:
	@echo '==============================================================='
	@echo ''
	cd palm ; ./configure --host=m68k-palmos --build=`../config.guess`

clean: X11/Makefile palm/Makefile
	cd X11 ; make $@
	cd gtk ; make $@
	cd OSX ; make $@
	cd palm ; make $@
	cd webos ; make $@

distclean: X11/Makefile gtk/Makefile palm/Makefile
	cd X11 ; make $@
	cd gtk ; make $@
	cd OSX ; make $@
	cd palm ; make $@
	cd webos ; make $@

install:
	@echo ''; \
	 echo '      - to install the X11 version: "cd X11; make install".';   \
	 echo ''; \
	 echo '      - to install the MacOS version: "cd OSX; make"';	     \
	 echo '        then copy "build/Development/DaliClock.app"';	     \
	 echo '        to your Applications folder."';			     \
	 echo ''; \
	 echo '      - to install the PalmOS version: "cd palm; make", and'; \
	 echo '        then download "daliclock.prc" to your PalmOS device.';\
	 echo ''; \
	 echo '      - to install the WebOS version: "cd webos; make", and'; \
	 echo '        then send the .ipk to your Palm WebOS device.';\
	 echo ''; \
	 exit 5

dmg::
	cd OSX ; $(MAKE) dmg

tar: X11/Makefile gtk/Makefile palm/Makefile dmg
	@								    \
  for dir in X11 gtk palm ; do						    \
   ( cd $$dir ;								    \
     rm -f configure ;							    \
     autoconf263 ;							    \
     ./config.status ;							    \
     $(MAKE) distdepend );						    \
  done ;								    \
  (cd X11; make update_spec_version) ;					    \
  (cd gtk; make update_spec_version) ;					    \
  (cd palm; make update_rsc_version) ;					    \
  (cd webos; make update_release_date) ;				    \
  (cd OSX; make update_plist_version) ;					    \
  NAME=`sed -n								    \
  's/[^0-9]*\([0-9]\.[0-9][0-9]*\).*/xdaliclock-\1/p' version.h` ;	    \
  rm -rf $$NAME ; ln -s . $$NAME ;					    \
  FILES= ;								    \
  for subdir in X11 gtk palm OSX webos ; do				    \
    d=`pwd` ;								    \
    cd $$subdir ;							    \
    FILES="$$FILES `$(MAKE) echo_tarfiles				    \
      | grep -v '^.*make\['						    \
      | sed \"s|^|$$subdir/|g;s| | $$subdir/|g\"			    \
      ` ";								    \
    cd $$d ; done ;							    \
  echo creating tar file archive/$${NAME}.tar.gz... ;			    \
  GZIP="-9v" $(TAR) -vczf archive/$${NAME}.tar.gz			    \
    `echo $(TARFILES) $$FILES $(NUMBERS)				    \
   | sed "s|^|$$NAME/|g; s| | $$NAME/|g" ` ;				    \
  rm $$NAME ;								    \
  ls -ldF archive/$${NAME}.tar.gz

www::
	@								    \
  DEST=$$HOME/www/xdaliclock ;						    \
  VERS=`sed -n 's/[^0-9]*\([0-9]\.[0-9][0-9]*\).*/\1/p' version.h`	 ;  \
  VERS2=`echo $$VERS | sed 's/[.]//g'`					 ;  \
  HEAD="daliclock-$$VERS"						 ;  \
  HEAD2="daliclock-$$VERS2"						 ;  \
  HEAD3="DaliClock-$$VERS2"						 ;  \
  TAR="x$$HEAD.tar.gz"							 ;  \
  DMG="$$HEAD3.dmg"							 ;  \
									    \
  if [ ! -f archive/$$TAR ]; then					    \
    echo "archive/$$TAR does not exist!  Did you forget to \`make tar'?" ;  \
    exit 1 ; 								    \
  fi ;									    \
									    \
  if [ ! -f archive/$$DMG ]; then					    \
    echo "archive/$$DMG does not exist!  Did you forget to \`make dmg'?" ;  \
    exit 1 ; 								    \
  fi ;									    \
  chmod a-w archive/$$TAR ;						    \
  chmod a-w archive/$$DMG ;						    \
									    \
  if [ -f $$DEST/$$TAR ]; then						    \
    /bin/echo -n "WARNING: $$DEST/$$TAR already exists!  Overwrite? ";	    \
    read line;								    \
    if [ "x$$line" != "xyes" -a "x$$line" != "xy" ]; then		    \
      exit 1 ; 								    \
    fi ;								    \
  fi ;									    \
									    \
  if [ -f $$DEST/$$DMG ]; then						    \
    /bin/echo -n "WARNING: $$DEST/$$DMG already exists!  Overwrite? ";	    \
    read line;								    \
    if [ "x$$line" != "xyes" -a "x$$line" != "xy" ]; then		    \
      exit 1 ; 								    \
    fi ;								    \
  fi ;									    \
									    \
  cp -p archive/$$TAR $$DEST/$$TAR ;					    \
  chmod u+w $$DEST/$$TAR ;						    \
									    \
  cp -p archive/$$DMG $$DEST/$$DMG ;					    \
  chmod u+w $$DEST/$$DMG ;						    \
									    \
  cp -p OSX/updates.xml $$DEST/ ;					    \
									    \
  cd $$DEST ;								    \
									    \
  TMP=/tmp/xd.$$$$ ;							    \
  sed -e "s/daliclock-[0-9]\.[0-9][0-9]*/$$HEAD/g"			    \
      -e "s/daliclock-[0-9][0-9][0-9]*/$$HEAD2/g"			    \
      -e "s/DaliClock-[0-9][0-9][0-9]*/$$HEAD3/g"			    \
      index.html > $$TMP ;						    \
  echo '' ;								    \
  diff -U0 index.html $$TMP ;						    \
  echo '' ;								    \
									    \
  for EXT in tar.gz dmg ; do						    \
    OLDEST=`ls *.$$EXT | fgrep -v 235.dmg | head -n 1` ;		    \
    /bin/echo -n "Delete $$DEST/$$OLDEST? ";				    \
    read line;								    \
    if [ "x$$line" = "xyes" -o "x$$line" = "xy" ]; then			    \
      set -x ;								    \
      rm $$OLDEST ;							    \
      cvs remove $$OLDEST ;						    \
      set +x ;								    \
    fi ;								    \
  done ;								    \
									    \
  cat $$TMP > index.html ;						    \
  rm -f $$TMP ;								    \
									    \
  set -x ;								    \
  cvs add -kb $$TAR $$DMG ;						    \
  cvs commit -m "$$VERS"

