CC = g++
CFLAGS = -Wall -Wextra
ODIR = obj
SDIR = ZAP
INC = -I../ZAP
EXE = ZAP

ifeq ($(OS),Windows_NT) 
    detected_OS := Windows
	EXE = ZAP.exe
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
	
endif

_OBJS = $(addsuffix .o, $(basename $(notdir $(wildcard $(SDIR)/*.cpp))))
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

run: $(EXE)
	./$(EXE)

$(ODIR)/%.o: $(SDIR)/%.cpp ZAP/debug.hpp
	$(CC) -c $(INC) -o $@ $< $(CFLAGS) 

$(EXE): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $(EXE)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(EXE)