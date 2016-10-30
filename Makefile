SDIR=src
CFLAGS=-std=c++14 -g
LIBS=
CC=g++
TT=~/projects/data/small_tt.xml

connections: $(SDIR)/main.o 
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

$(SDIR)/main.o: $(SDIR)/main.cpp $(SDIR)/headers.hpp.gch
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(SDIR)/headers.hpp.gch: $(SDIR)/headers.hpp
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm connections $(SDIR)/*.o $(SDIR)/*.gch

run: connections
	./connections $(TT)