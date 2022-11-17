# Note that this is NOT a relocatable package
%define name Maelstrom
%define version 3.0.6
%define release 1
%define prefix /usr

Summary: Simple DirectMedia Layer
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.gz
URL: http://www.devolution.com/~slouken/Maelstrom/
Copyright: GPL
Group: Games
BuildRoot: /var/tmp/%{name}-buildroot

%description
Maelstrom is a rockin' asteroids game ported from the Macintosh
Originally written by Andrew Welch of Ambrosia Software, and ported
to UNIX and then SDL by Sam Lantinga <slouken@devolution.com>

%prep
rm -rf ${RPM_BUILD_ROOT}

%setup -q

%build
# Needed for snapshot releases.
if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=%prefix
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix
fi

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
rm -rf $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/%{prefix}

%clean
rm -rf $RPM_BUILD_ROOT

%post
# Add desktop menu bar items
function Add_DeskTop_MenuItem
{
    desktop=$1; deskfile=$2
    if [ -d "$desktop" ]; then
        desktop="$desktop/Games"
        if [ ! -d "$desktop" ]; then
            mkdir "$desktop" 2>/dev/null
        fi
        if [ -w "$desktop" ]; then
            echo "Creating $desktop/$deskfile"
            cat >"$desktop/$deskfile" <<__EOF__
# KDE Config File
[KDE Desktop Entry]
Name=Maelstrom
Comment=Maelstrom
Exec=/usr/bin/Maelstrom
Icon=/usr/games/Maelstrom/icon.xpm
Terminal=0
Type=Application
__EOF__
        fi
    fi
}
echo "============================================================="
echo "Adding desktop menu items ..."
for gnomedir in "/opt/gnome" "/usr/share/gnome" "$HOME/.gnome"
do Add_DeskTop_MenuItem "$gnomedir/apps" "maelstrom.desktop"
done
for kdedir in "/opt/kde" "/usr/share/kde" "$HOME/.kde"
do Add_DeskTop_MenuItem "$kdedir/share/applnk" "maelstrom.kdelnk"
done

%postun
echo "============================================================="
echo "Removing desktop menu items ..."
for gnomedir in "/opt/gnome" "/usr/share/gnome" "$HOME/.gnome"
do rm -f "$gnomedir/apps/Games/maelstrom.desktop"
done
for kdedir in "/opt/kde" "/usr/share/kde" "$HOME/.kde"
do rm -f "$kdedir/share/applnk/Games/maelstrom.kdelnk"
done

%files
%defattr(-, root, root)
%doc COPYING* CREDITS README* Changelog Docs
%{prefix}/bin/Maelstrom
%{prefix}/games/Maelstrom

%changelog
* Tue Sep 21 1999 Sam Lantinga <slouken@devolution.com>

- first attempt at a spec file

