
CFLAGS = $(OPTIMIZE) -I.. $(INCLUDES) -DNETPLAY
OBJS = about.o blit.o game.o logic.o make.o netplay.o object.o player.o

netlogic.o: $(OBJS)
	ld -r -o $@ $(OBJS)

clean:
	rm -f *.o *.bak

include .c++howto
