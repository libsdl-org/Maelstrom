

PROG = netplayd
OBJS = netplayd.o
CFLAGS = -g -I../netlogic

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) *.bak

spotless: clean
	rm -f $(PROG)
