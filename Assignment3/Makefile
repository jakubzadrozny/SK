CC = cc
CFLAGS = -std=gnu99 -Wall -Wextra

BIN = transport
OBJS = packets.o handler.o

$(BIN): $(OBJS) $(BIN).c
	$(CC) $(CFLAGS) $(OBJS) $(BIN).c -o $(BIN)

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o

distclean:
	rm -f *.o $(BIN)
