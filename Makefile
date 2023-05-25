EXES = art_parser art_search
OBJECTS = art_parser.o art_search.o
CFLAGS = -g -std=gnu11
all: $(EXES)

art_parser : art_parser.o
	gcc -o art_parser $(CFLAGS) art_parser.o
art_search : art_search.o
	gcc -o art_search $(CFLAGS) art_search.o

$(OBJECTS) : 
.PHONY : clean
clean :
	rm $(EXES) $(OBJECTS)
