CC=gcc
CFLAGS=-c 
LDFLAGS=
SOURCES=bitreader.c tsparser.c tsunpacker.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=tsunpacker

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o tsunpacker
