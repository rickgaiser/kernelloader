PROGRAM = png2rgb

CFLAGS = -g -Wall -Werror-implicit-function-declaration
LDFLAGS = -lpng

all: $(PROGRAM)

png2rgb.o: png2rgb.c

clean:
	rm -f $(PROGRAM) png2rgb.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

%: %.o
	$(CC) -o $@ $^ $(LDFLAGS)
