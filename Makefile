OBJS = timeouts.o signals.o poll.o poll_select.o socket.o
CPPFLAGS = -DASM
CFLAGS = -Wall -g -O0 

all: libevent.a event

event: libevent.a test.o
	$(CC) -o test test.o libevent.a

libevent.a: $(OBJS)
	ar -rvc libevent.a $(OBJS)

test.o: test.c	
	$(CC) -g -O0 -c -o test.o test.c

clean:
	rm -f $(OBJS) test libevent.a
