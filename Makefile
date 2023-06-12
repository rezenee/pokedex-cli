EXES = art_parser art_search json_parser
OBJECTS = art_parser.o art_search.o json_parser.o
CFLAGS = -g -std=gnu11
all: $(EXES)

json_parser : json_parser.o
	gcc -o json_parser cJSON.c $(CFLAGS) json_parser.o
art_parser : art_parser.o
	gcc -o art_parser $(CFLAGS) art_parser.o
art_search : art_search.o
	gcc -o art_search $(CFLAGS) art_search.o

$(OBJECTS) : 
.PHONY : clean
clean :
	rm $(EXES) $(OBJECTS)
