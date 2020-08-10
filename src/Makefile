#
# Makefile

INCDIR=/usr/include
LIBDIR=/usr/lib64

GCC = gcc
CFLAGS = -g -O4 -W -Wextra -D_FORTIFY_SOURCE=2
#CFLAGS = -g -DDEBUG -Wall -Wextra
#CFLAGS = -O3 -Wall -DDEBUG

LIBS = -lpthread -lm
TARGETS = gentree readtree tt

all: $(TARGETS)

gentree: gentree.o genfile.o readparms.o timer.o rand.o fifo.o
	$(GCC) $(CFLAGS) -o $@ $? $(LIBS)

readtree: readtree.o readparms.o timer.o rand.o fifo.o
	$(GCC) $(CFLAGS) -o $@ $? $(LIBS)

tt: tt.o
	$(GCC) $(CFLAGS) -o $@ $? $(LIBS)

-include $(OBJS:.o=.d)

%.o: %.c
	$(GCC) $(CFLAGS) -I $(INCDIR) -c $<
	$(GCC) $(CFLAGS) -I $(INCDIR) -MM $< > $*.d

clean:
	rm -f $(TARGETS) *.d *.o *~ *#
