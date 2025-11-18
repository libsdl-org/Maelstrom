
ACLOCAL_AMFLAGS = -I acinclude

bin_PROGRAMS = Maelstrom
noinst_PROGRAMS = MaelstromLobby

Maelstrom_SOURCES = mainstub.cpp

Maelstrom_LDADD = 			\
	game/libgame.a		\
	screenlib/libSDLscreen.a	\
	maclib/libSDLmac.a		\
	physfs/libphysfs.a		\
	utils/libutils.a		\
	@EXTRA_LIBS@ @ICONOBJ@

MaelstromLobby_SOURCES = MaelstromLobby.cpp

MaelstromLobby_LDADD = @EXTRA_LIBS@

SUBDIRS = game screenlib maclib physfs utils

EXTRA_DIST =			\
	Docs			\
	COPYING			\
	COPYING.GPL		\
	CREDITS			\
	Changelog		\
	Maelstrom.spec		\
	Maelstrom_Data.zip	\
	README.joystick		\
	README.network		\
	README.options		\
	README.simple		\
	VisualC			\
	Xcode			\
	Xcode-iOS		\
	autogen.sh		\
	ico_o			\
	icon.xpm

DIST_SUBDIRS = $(SUBDIRS)

all: data

data: Maelstrom_Data.zip

append-data:
	cat Maelstrom_Data.zip >>$(if $(BINARY),$(BINARY),Maelstrom$(EXEEXT))

Maelstrom_Data.zip: $(shell find Data -type f -print | fgrep -v .svn)
	(cd Data && find . -type f | fgrep -v .svn | zip -r ../$@.new -@ && mv ../$@.new ../$@)

install-exec-hook:
	make append-data BINARY="$(DESTDIR)$(bindir)/Maelstrom$(EXEEXT)"

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpm -ta $(PACKAGE)-$(VERSION).tar.gz

.PHONY: append-data rpm
