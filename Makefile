OBJS = timeouts.o signals.o poll.o poll_select.o socket.o
CPPFLAGS = -DASM
CFLAGS = -Wall -g -O0 -I../ebtree

all: libevents.a

libevents.a: $(OBJS)
	ar -rvc libevents.a ../ebtree/ebtree.a $(OBJS)

clean:
	rm -f $(OBJS) libevents.a

doc:
	doxygen events.doxygen

cleandoc:
	rm -rf html
