SHELL=/bin/sh

MAELSTROM = Maelstrom Maelstrom_sound

############################################################################

include Makeflags

############################################################################


install: $(MAELSTROM)
	@if [ "$(LIBDIR)" = "." ]; then \
		echo "	Nothing to do!  (Install to current directory)"; \
		exit 1; \
	fi
	@if [ ! -d "$(BINDIR)" ]; then mkdir $(BINDIR); fi
	@echo "Installing binaries in $(BINDIR)..."
	@for i in $(MAELSTROM) ; do \
		strip $$i; \
		$(INSTALL) -m 755 $$i $(BINDIR); \
	done
	@if [ ! -d $(LIBDIR) ]; then \
		mkdir $(LIBDIR); chmod 755 $(LIBDIR); \
	fi
	@echo "Installing data files in $(LIBDIR) ..."
	@tar cf - $(DATAFILES) | (cd $(LIBDIR); tar xvf -)
	@chmod -R +r $(LIBDIR)
	@if [ ! -f "$(LIBDIR)/Maelstrom-Scores" ]; then \
		echo "Installing Maelstrom high-scores file"; \
		$(INSTALL) -m 666 Maelstrom-Scores  $(LIBDIR); \
		chmod 666 $(LIBDIR)/Maelstrom-Scores; \
	fi
	@-if [ "$(SYSTEM)" = "Linux" ]; then \
		echo "Fixing permissions for Maelstrom (set-uid root)"; \
		chown root $(BINDIR)/Maelstrom && \
					chmod u+s $(BINDIR)/Maelstrom; \
	fi
	@echo "Maelstrom installed!"
	@echo ""
	@echo "Make sure that $(BINDIR) is in your execution path"
	@echo "and type 'Maelstrom' to play!"

# I've always really disliked programs that installed themselves
# all over my system and didn't show me how to remove them.
uninstall:
	@if [ -x $(BINDIR)/Maelstrom ]; then \
		echo "Removing Maelstrom binaries..."; \
		rm $(BINDIR)/Maelstrom*; \
	fi
	@if [ -d $(LIBDIR) ]; then \
		if [ -f $(LIBDIR)/Maelstrom-Scores ]; then \
			if [ `echo "\c"` = "" ]; then \
				echo "Save high scores? [Y/n]: \c"; \
			elif [ `echo -n ""` = ""]; then \
				echo -n "Save high scores? [Y/n]: "; \
			else \
				echo "Save high scores? [Y/n]"; \
			fi; \
			read savem; \
			if [ "$$savem" != "n" ]; then \
				cp $(LIBDIR)/Maelstrom-Scores . ; \
				echo \
				"High scores saved as ./Maelstrom-Scores"; \
			fi; \
		fi; \
		echo "Removing data files..."; \
		rm -r $(LIBDIR); \
	fi
	@echo "Maelstrom uninstalled!"

config:
	sh ./Configure.sh

Maelstrom: $(OBJS)
	$(CXX) $(OPTIMIZE) -o $@ $(OBJS) $(CSUMLIBS) $(LIBS)

Maelstrom_sound: $(SOUNDOBJS)
	$(CXX) $(OPTIMIZE) -o $@ $(SOUNDOBJS) $(SOUNDLIBS)

$(LOGIC)/$(LOGIC).o:
	if [ "$(LOGIC)" != "" ]; then \
		cd "$(LOGIC)" && \
		make "CXX=$(CXX)" "OPTIMIZE=$(OPTIMIZE)" "INCLUDES=$(INCLUDES)"; \
	fi

macres: macres.o Mac_Resource.o Mac_Resource.h
	$(CXX) $(OPTIMIZE) -o $@ macres.o Mac_Resource.o

clean:
	if [ "$(LOGIC)" != "" ]; then cd $(LOGIC) && make clean; fi
	rm -f *.o *.bak gmon.out
	-cd netplayd && make clean

dist: clean
	cp Makeflags.start Makeflags
	strip $(MAELSTROM)

spotless: clean
	if [ "$(MAELSTROM)" != "" ]; then rm -f $(PROGS) $(MAELSTROM); fi
	rm -f mixer.h mixer.cpp
	cp Makeflags.start Makeflags
	-cd netplayd && make spotless

clobber: spotless


Maelstrom_Globals.h: Mac_Resource.h sound.h fontserv.h framebuf.h \
                     x11_framebuf.h vga_framebuf.h dga_framebuf.h \
                     			Maelstrom.h Maelstrom_Inline.h
	@if [ -f "$@" ]; then touch "$@"; fi

# DO NOT DELETE
#
# The SGI needs this:
include .c++howto
