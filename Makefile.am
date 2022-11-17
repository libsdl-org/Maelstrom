

bin_PROGRAMS = Maelstrom Maelstrom-netd

Maelstrom_SOURCES =		\
	Maelstrom.h		\
	Maelstrom_Globals.h	\
	buttonlist.h		\
	checksum.cpp		\
	checksum.h		\
	colortable.h		\
	controls.cpp		\
	controls.h		\
	dialog.cpp		\
	dialog.h		\
	fastrand.cpp		\
	fastrand.h		\
	init.cpp		\
	load.cpp		\
	load.h			\
	logic.h			\
	main.cpp		\
	myerror.cpp		\
	myerror.h		\
	netscore.cpp		\
	netscore.h		\
	public_key.h		\
	rect.cpp		\
	rect.h			\
	scores.cpp		\
	scores.h

LOGIC = netlogic

Maelstrom_LDADD = 			\
	$(LOGIC)/liblogic.a		\
	screenlib/libSDLscreen.a	\
	maclib/libSDLmac.a		\
	@MATHLIB@ @INETLIB@ @ICONOBJ@

SUBDIRS = $(LOGIC) screenlib maclib

EXTRA_DIST =			\
	autogen.sh		\
	VisualC.zip		\
	CREDITS.txt		\
	Changelog		\
	README.txt		\
	README.joystick.txt	\
	README.network.txt	\
	README.options.txt	\
	COPYING.txt		\
	COPYING.GPL		\
	COPYING.GPL.txt		\
	Maelstrom_Fonts		\
	Maelstrom_Sounds	\
	Maelstrom_Sprites	\
	ico_o			\
	icon.bmp icon.xpm	\
	Maelstrom.entitlements \
	Maelstrom.spec

DIST_SUBDIRS = $(SUBDIRS) Images Docs

# Special install rule for the game
install:
	sh mkinstalldirs $(DESTDIR)@GAME_INSTALLDIR@
	install -m 755 Maelstrom $(DESTDIR)@GAME_INSTALLDIR@/Maelstrom
	make install_gamedata target=$(DESTDIR)@GAME_INSTALLDIR@
	make install_gamedocs target=$(DESTDIR)@GAME_INSTALLDIR@

install_gamedata:
	sh mkinstalldirs $(target)/
	sh mkinstalldirs $(target)/Images
	cp -v Images/*.bmp $(target)/Images
	cp -v Images/*.cicn $(target)/Images
	cp -v Maelstrom_Fonts	\
	      Maelstrom_Sounds	\
	      Maelstrom_Sprites	\
	      icon.bmp icon.xpm	$(target)/

install_gamedocs:
	sh mkinstalldirs $(target)/
	sh mkinstalldirs $(target)/Docs
	cp -v Docs/*.txt $(target)/Docs
	cp -v README* $(target)/
	cp -v COPYING.txt $(target)/COPYING
	cp -v COPYING.GPL $(target)/

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz
