OBJS = timeouts.o signals.o poll.o poll_select.o socket.o
CPPFLAGS = -DASM
CFLAGS = -Wall -g -O0 

all: libevents.a test

test: libevents.a test.o
	$(CC) -o test test.o libevents.a

libevents.a: $(OBJS)
	ar -rvc libevents.a $(OBJS)

clean:
	rm -f $(OBJS) test libevents.a

doc:
	doxygen events.doxygen

