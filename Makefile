EXES = pre_parser parser_a2 search_a2 parser_a1 csv_shrink
SRCS = pre_parser.c parser_a2.c search_a2.c parser_a1.c csv_shrink.c
CFLAGS = -g -std=gnu11

all: $(EXES)

csv_shrink : csv_shrink.c
	gcc $(CFLAGS) -o csv_shrink csv_shrink.c && rm -f csv_shrink.o
search_a2 : search_a2.c
	gcc $(CFLAGS) -o search_a2 search_a2.c && rm -f search_a2.o
parser_a2 : parser_a2.c
	gcc $(CFLAGS) -o parser_a2 parser_a2.c && rm -f parser_a2.o
parser_a1: parser_a1.c
	gcc $(CFLAGS) -o parser_a1 parser_a1.c && rm -f parser_a1.o
pre_parser : pre_parser.c
	gcc $(CFLAGS)  -o pre_parser pre_parser.c data/cJSON.c -lcurl && rm -f pre_parser.o

.PHONY : clean
clean :
	rm -f $(EXES) $(SRCS:.c=.o) *.bin *.csv
