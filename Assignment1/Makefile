CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra

BIN = traceroute
OBJS = traceroute.o packet_tools.o handler.o

$(BIN): $(OBJS) main.c
	$(CC) $(CFLAGS) $(OBJS) main.c -o $(BIN)

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o

distclean:
	rm -f *.o $(BIN)
