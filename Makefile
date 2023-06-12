EXES = art_parser art_search json_parser
SRCS = art_parser.c art_search.c json_parser.c
CFLAGS = -g -std=gnu11

all: $(EXES)

json_parser : json_parser.c
	gcc $(CFLAGS) -o json_parser json_parser.c cJSON.c && rm -f json_parser.o

art_parser : art_parser.c
	gcc $(CFLAGS) -o art_parser art_parser.c && rm -f art_parser.o

art_search : art_search.c
	gcc $(CFLAGS) -o art_search art_search.c && rm -f art_search.o

.PHONY : clean
clean :
	rm -f $(EXES) $(SRCS:.c=.o)
