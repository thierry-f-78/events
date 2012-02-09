OBJS = signals.o poll.o poll_select.o socket.o
CFLAGS = -Wall -g -O3 -I../ebtree

all: libevents.a

libevents.a: $(OBJS)
	ar -rvc libevents.a $(OBJS)

clean:
	rm -f $(OBJS) libevents.a

doc:
	doxygen events.doxygen

cleandoc:
	rm -rf html
