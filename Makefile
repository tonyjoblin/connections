SDIR=src
CFLAGS=-std=c++14 -g
LIBS=-lboost_date_time
CC=g++
TT=~/projects/data/small_tt.xml

connections: $(SDIR)/main.o
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

$(SDIR)/main.o: $(SDIR)/main.cpp $(SDIR)/headers.hpp.gch Makefile
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(SDIR)/headers.hpp.gch: $(SDIR)/headers.hpp Makefile
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm connections $(SDIR)/*.o $(SDIR)/*.gch

test: connections
	time ./connections $(TT)

run: connections
	time ./connections ~/projects/data/20160530020814_v7.xml > c.bsv
