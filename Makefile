OBJS = timeouts.o signals.o poll.o poll_select.o socket.o
CPPFLAGS = -DASM
CFLAGS = -Wall -g -O0 

all: libevents.a

libevents.a: $(OBJS)
	ar -rvc libevents.a $(OBJS)

clean:
	rm -f $(OBJS) libevents.a

doc:
	doxygen events.doxygen

cleandoc:
	rm -rf html
