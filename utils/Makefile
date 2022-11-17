
PROGS = dsprite dicon dppm xpm2icon xpm2sprite clut2cmap \
	mergescores printscores gen_vgakeys data2array matchcolors endianicon \
	populatecmap unpopulatecmap genkeypair countcolors

all: $(PROGS)

dsprite: dsprite.o ../colortable.h ../Sprite.h
	$(CC) -o $@ $< -lvga -lvgagl

dicon: dicon.o ../colortable.h
	$(CC) -o $@ $< -lvga -lvgagl

dppm: dppm.o
	$(CC) -o $@ $< -lvga -lvgagl

cicn2xpm: cicn2sprite.c colortable.h ../bytesex.h Sprite.h
	$(CC) -o $@ $< -DMAKE_XPM 

countcolors: countcolors.o ../Mac_Resource.o
	$(CC) -o $@  countcolors.o ../Mac_Resource.o

unpopulatecmap: populatecmap.c
	$(CC) -o $@ $<

clean:
	rm -f *.o

spotless: clean
	rm -f $(PROGS) *.bak
