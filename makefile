CC = g++
CFLAGS = -Wall -Wextra
TEST_ODIR = test/obj
TEST_SDIR = test
TEST_INC = -I../test
TEST_EXE = ZAP_TEST
ODIR = ZAP/obj
SDIR = ZAP
INC = -I../ZAP
EXE = ZAP

ifeq ($(OS),Windows_NT) 
    detected_OS := Windows
	EXE = ZAP.exe
	TEST_EXE = ZAP_TEST.exe
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
	
endif

_OBJS = $(addsuffix .o, $(basename $(notdir $(wildcard $(SDIR)/*.cpp))))
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_TEST_OBJS = $(addsuffix .o, $(basename $(notdir $(wildcard $(TEST_SDIR)/*.cpp))))
TEST_OBJS = $(patsubst %,$(TEST_ODIR)/%,$(_TEST_OBJS))

run: $(EXE)
	./$(EXE)

$(ODIR)/%.o: $(SDIR)/%.cpp ZAP/debug.hpp
	$(CC) -c $(INC) -o $@ $< $(CFLAGS) 

$(EXE): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $(EXE)

test: $(TEST_EXE) $(EXE)
	./$(TEST_EXE) $(EXE)

$(TEST_ODIR)/%.o: $(TEST_SDIR)/%.cpp
	$(CC) -c $(INC) -o $@ $< $(CFLAGS) 

$(TEST_EXE): $(TEST_OBJS) 
	$(CC) $(CFLAGS) $(TEST_OBJS) -o $(TEST_EXE)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o $(EXE)
	rm -f $(TEST_ODIR)/*.o $(TEST_EXE)