

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
	PBProjects.tar.gz	\
	MPWmake.sea.bin		\
	CREDITS			\
	Changelog		\
	README.joystick		\
	README.network		\
	README.options		\
	COPYING			\
	COPYING.GPL		\
	Images			\
	Maelstrom_Fonts		\
	Maelstrom_Sounds	\
	Maelstrom_Sprites	\
	Maelstrom-Scores	\
	ico_o			\
	icon.bmp icon.xpm	\
	Maelstrom.spec

DIST_SUBDIRS = $(SUBDIRS) Images Docs

# Special install rule for the game
install:
	make install_gamedata target=@GAME_INSTALLDIR@

install_gamedata:
	sh mkinstalldirs $(target)/
	cp -rv Images			\
	       Maelstrom_Fonts		\
	       Maelstrom_Sounds		\
	       Maelstrom_Sprites	\
	       Maelstrom-Scores		\
	       icon.bmp icon.xpm	$(target)/
	chmod 666 $(target)/Maelstrom-Scores

install_gamedocs:
	sh mkinstalldirs $(target)/
	cp -rv README* COPYING* Docs*	$(target)/

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz
