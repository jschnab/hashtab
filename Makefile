CC = gcc
CFLAGS = -Wall
LDFLAGS = -lm
hashtab: main.o hash_table.o prime.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
