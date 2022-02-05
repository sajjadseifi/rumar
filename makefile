CC=gcc
CFLAGS=-I.
OBJ = peer.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
clear:
	rm -r *.o