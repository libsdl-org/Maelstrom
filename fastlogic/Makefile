
CFLAGS = $(OPTIMIZE) -I.. $(INCLUDES)
OBJS = logic.o blit.o game.o make.o about.o

fastlogic.o: $(OBJS)
	ld -r -o $@ $(OBJS)

clean:
	rm -f *.o *.bak

include .c++howto
