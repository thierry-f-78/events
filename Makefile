OBJS = timeouts.o signals.o poll.o poll_select.o test.o
CPPFLAGS = -DASM
CFLAGS = -Wall -g -O0 

#libevent.so: $(OBJS)

event: $(OBJS)
	$(CC) -o event $(OBJS)

test.o: test.c	
	$(CC) -g -O0 -c -o test.o test.c

clean:
	rm -f $(OBJS)
