# Makefile for echo client and server

CXX=	g++ $(CCFLAGS)

MSG-SERVER=	server.o messages.o order_queue.o
MSG-CLIENT=	client.o
OBJS =		$(MSG-SERVER) $(MSG-CLIENT)

LIBS =	-pthread -std=c++11

CCFLAGS = -g

all:	msg-server msg-client

msg-server:$(MSG-SERVER)
	$(CXX) -o server $(MSG-SERVER) $(LIBS)

msg-client:$(MSG-CLIENT)
	$(CXX) -o client $(MSG-CLIENT) $(LIBS)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)

realclean:
	rm -f $(OBJS) $(OBJS:.o=.d) server client


# These lines ensure that dependencies are handled automatically.
%.d:	%.cc
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

include	$(OBJS:.o=.d)
