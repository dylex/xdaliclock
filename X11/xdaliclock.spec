Name: xdaliclock
Summary: melting digital clock
Vendor: Jamie Zawinski <jwz@jwz.org>
Version: 2.19
Release: 1
URL: http://www.jwz.org/xdaliclock/
Source: xdaliclock-%{version}.tar.gz
Copyright: BSD
Group: X11/Utilities
Buildroot: /var/tmp/xdaliclock-root

%description
The xdaliclock program displays a digital clock; when a digit changes, it
"melts" into its new shape.

It can display in 12 or 24 hour modes, and displays the date when a mouse
button is held down.  It has two large fonts built into it, but it can animate
other fonts.  Funky psychedelic colormap cycling is also supported.

%prep
%setup -q
%build

xmkmf
make depend
make

%install

root=$RPM_BUILD_ROOT/usr/X11R6

mkdir -p $root/bin
mkdir -p $root/lib/X11/app-defaults
mkdir -p $root/man/man1
mkdir -p $RPM_BUILD_ROOT/etc/X11/wmconfig

install -m 755 xdaliclock     $root/bin
install -m 644 XDaliClock.ad  $root/lib/X11/app-defaults/XDaliClock
install -m 644 xdaliclock.man $root/man/man1/xdaliclock.1x

# This is for wmconfig, a tool that generates init files for window managers.
#
mkdir -p $RPM_BUILD_ROOT/etc/X11/wmconfig
cat > $RPM_BUILD_ROOT/etc/X11/wmconfig/xdaliclock <<EOF
xdaliclock name "xdaliclock"
xdaliclock description "Dali Clock"
xdaliclock group "Amusements"
xdaliclock exec "xdaliclock -cycle &"
EOF

# This is for the GNOME desktop:
#
mkdir -p "$RPM_BUILD_ROOT/usr/share/apps/Amusements/"
cat > "$RPM_BUILD_ROOT/usr/share/apps/Amusements/xdaliclock.desktop" <<EOF
[Desktop Entry]
Name=xdaliclock
Description=Dali Clock
Exec=xdaliclock -cycle &
Terminal=false
Type=Application
EOF

# Make sure all files are readable by all, and writable only by owner.
#
chmod -R a+r,u+w,og-w $RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)

%doc                README
                    /usr/X11R6/bin/*
%config             /usr/X11R6/lib/X11/app-defaults/*
                    /usr/X11R6/man/man1/*
%config(missingok)  /etc/X11/wmconfig/*
%config(missingok)  "/usr/share/apps/Amusements/*"
