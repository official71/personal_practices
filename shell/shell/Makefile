CC := gcc
CFLAGS := -Wall -Werror -g
LDFLAGS :=


OBJECTS :=  shell.o main.o

all: w4118_sh

w4118_sh: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f w4118_sh
	rm -f shell.o
	rm -f main.o

.PHONY: clean
