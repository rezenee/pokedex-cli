EXE = pokedex
OBJECTS = pokedex.o
CFLAGS = -g -std=gnu11
$(EXE) : $(OBJECTS) 
	gcc -o $(EXE) $(CFLAGS) $(OBJECTS) 

$(OBJECTS) : 
.PHONY : clean
clean :
	rm $(EXE) $(OBJECTS)
