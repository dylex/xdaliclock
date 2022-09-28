# xdaliclock, Copyright (c) 1991-2022 by Jamie Zawinski.

SHELL		= /bin/sh
TARFILES	= README Makefile version.h \
		  mac128/README mac128/*.*
NUMBERS		= font/dalifont*.{gif,png,ai,psd.gz} font/*.{c,xbm}
TAR		= gnutar
TAR_ARGS	= --owner=0 --group=0 --posix --no-acls --no-xattrs --no-selinux


default:
	@echo ''; \
	 echo '        please make one of: "x", "osx", "palm", "webos", "android", "all", or "clean"'; \
	 echo ''; \
	 exit 5

all:: OSX webos palm X11 android

x::     _x11
osx::   _osx
palm::  _Palm
webos:: _WebOS
android:: _Android

_x11:: X11/Makefile
	@echo '==============================================================='
	@echo ''
	cd X11 ; make

_classic:: X11/Xlib-classic/Makefile
	@echo '==============================================================='
	@echo ''
	cd X11/Xlib-classic ; make

_osx::
	@echo '==============================================================='
	@echo ''
	cd OSX ; make

_Palm:: palm/Makefile
	@echo '==============================================================='
	@echo ''
	cd palm ; make

_WebOS:: webos/Makefile
	@echo '==============================================================='
	@echo ''
	cd webos ; make

_pebble::
	@echo '==============================================================='
	@echo ''
	cd pebble ; pebble build

_Android:: android/Makefile
	@echo '==============================================================='
	@echo ''
	cd android ; make

X11/Makefile:
	@echo '==============================================================='
	@echo ''
	cd X11 ; ./configure

X11/Xlib-classic/Makefile:
	@echo '==============================================================='
	@echo ''
	cd X11/Xlib-classic ; ./configure

palm/Makefile:
	@echo '==============================================================='
	@echo ''
	cd palm ; ./configure --host=m68k-palmos --build=`../config.guess`

clean: X11/Makefile palm/Makefile
	cd X11 ; make $@
	cd X11/Xlib-classic ; make $@
	cd OSX ; make $@
	cd palm ; make $@
	cd webos ; make $@
	cd pebble ; make $@
	cd android ; make $@

distclean: X11/Makefile X11/Xlib-classic/Makefile palm/Makefile
	cd X11 ; make $@
	cd X11/Xlib-classic ; make $@
	cd OSX ; make $@
	cd palm ; make $@
	cd webos ; make $@
	cd pebble ; make $@
	cd android ; make $@

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
	 echo '      - to install the android version: "cd android; make", and'; \
	 echo '        then send the .apk to your android device.';\
	 echo ''; \
	 exit 5

dmg::
	cd OSX ; $(MAKE) dmg

apk::
	cd android ; $(MAKE) apk

tar: X11/Makefile X11/Xlib-classic/Makefile palm/Makefile apk _tar
_tar:
	@								    \
  for dir in X11 X11/Xlib-classic palm ; do				    \
   ( cd $$dir ;								    \
     echo "#### $$dir" ;						    \
     echo rm -f configure ;						    \
     echo autoconf263 ;							    \
     ./config.status ;							    \
     echo $(MAKE) distdepend );						    \
  done ;								    \
  (cd X11; $(MAKE) update_spec_version) ;				    \
  (cd palm; $(MAKE) update_rsc_version) ;				    \
  (cd webos; $(MAKE) update_release_date) ;				    \
  (cd OSX; $(MAKE) update_plist_version) ;				    \
  (cd android; $(MAKE) update_gradle_version update_gradle_app_id) ;	    \
  NAME=`sed -n								    \
  's/[^0-9]*\([0-9]\.[0-9][0-9]*\).*/xdaliclock-\1/p' version.h` ;	    \
  rm -rf $$NAME ; ln -s . $$NAME ;					    \
  FILES= ;								    \
  for subdir in X11 X11/Xlib-classic palm OSX webos pebble android ; do	    \
    d=`pwd` ;								    \
    cd $$subdir ;							    \
    FILES2="`$(MAKE) echo_tarfiles					    \
      | grep -v '^.*make\['						    \
      | sed \"s|^|$$subdir/|g;s| | $$subdir/|g\"			    \
      ` ";								    \
    FILES="$$FILES $$FILES2" ;						    \
    cd $$d ; done ;							    \
  echo "####" creating tar file archive/$${NAME}.tar.gz... ;		    \
  export COPYFILE_DISABLE=true ;					    \
  GZIP="-9v" $(TAR) --check-links -vczf archive/$${NAME}.tar.gz $(TAR_ARGS) \
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
  APK="$$HEAD3.apk"							 ;  \
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
									    \
  if [ ! -f archive/$$APK ]; then					    \
    echo "archive/$$APK does not exist!  Did you forget to \`make apk'?" ;  \
    exit 1 ; 								    \
  fi ;									    \
  chmod a-w archive/$$TAR ;						    \
  chmod a-w archive/$$DMG ;						    \
  chmod a-w archive/$$APK ;						    \
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
  if [ -f $$DEST/$$APK ]; then						    \
    /bin/echo -n "WARNING: $$DEST/$$APK already exists!  Overwrite? ";	    \
    read line;								    \
    if [ "x$$line" != "xyes" -a "x$$line" != "xy" ]; then		    \
      exit 1 ; 								    \
    fi ;								    \
  fi ;									    \
									    \
  ( cd $$DEST ;	git pull . ) ;						    \
									    \
  cp -p archive/$$TAR $$DEST/$$TAR ;					    \
  chmod u+w $$DEST/$$TAR ;						    \
									    \
  cp -p archive/$$DMG $$DEST/$$DMG ;					    \
  chmod u+w $$DEST/$$DMG ;						    \
									    \
  cp -p archive/$$APK $$DEST/$$APK ;					    \
  chmod u+w $$DEST/$$APK ;						    \
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
  for EXT in tar.gz dmg apk ; do					    \
    OLDEST=`ls *.$$EXT |						    \
      fgrep -v 235.dmg |						    \
      fgrep -v 243.dmg |						    \
      head -n 1` ;							    \
    /bin/echo -n "Delete $$DEST/$$OLDEST? ";				    \
    read line;								    \
    if [ "x$$line" = "xyes" -o "x$$line" = "xy" ]; then			    \
      set -x ;								    \
      rm $$OLDEST ;							    \
      git rm $$OLDEST ;							    \
      set +x ;								    \
    fi ;								    \
  done ;								    \
									    \
  cat $$TMP > index.html ;						    \
  rm -f $$TMP ;								    \
									    \
  set -x ;								    \
  git add index.html updates.xml $$TAR $$DMG $$APK ;			    \
  git commit -m "$$VERS" ;						    \
  git push
