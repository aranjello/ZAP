CC = g++
CFLAGS = -Wall -Wextra

TEST_ODIR = test/obj
TEST_SDIR = test
TEST_INC = -I../test

ODIR = ZAP/obj/release
DEBUG_ODIR = ZAP/obj/debug

SDIR = ZAP
INC = -I../ZAP

EXE = ZAP
TEST_EXE = ZAP_TEST
DEBUG_EXE = ZAP_DEBUG

ifeq ($(OS),Windows_NT) 
    detected_OS := Windows
	EXE = ZAP.exe
	TEST_EXE = ZAP_TEST.exe
	DEBUG_EXE = ZAP_DEBUG.exe
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
	
endif

_OBJS = $(addsuffix .o, $(basename $(notdir $(wildcard $(SDIR)/*.cpp))))
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
DEBUG_OBJS = $(patsubst %,$(DEBUG_ODIR)/%,$(_OBJS))

_TEST_OBJS = $(addsuffix .o, $(basename $(notdir $(wildcard $(TEST_SDIR)/*.cpp))))
TEST_OBJS = $(patsubst %,$(TEST_ODIR)/%,$(_TEST_OBJS))

debug: CFLAGS += -D DEBUG_TOKEN_CREATION
debug: build/debug/$(DEBUG_EXE)
	./build/debug/$(DEBUG_EXE)

release: build/release/$(EXE)
	./build/release/$(EXE)

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c $(INC) -o $@ $< $(CFLAGS)

$(DEBUG_ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c $(INC) -o $@ $< $(CFLAGS)

build/debug/$(DEBUG_EXE): $(DEBUG_OBJS) 
	$(CC) $(CFLAGS) $(DEBUG_OBJS) -o build/debug/$(DEBUG_EXE)

build/release/$(EXE): $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o build/release/$(EXE) 

test: $(TEST_EXE) build/debug/$(DEBUG_EXE)
	./$(TEST_EXE) build\debug\$(DEBUG_EXE)

$(TEST_ODIR)/%.o: $(TEST_SDIR)/%.cpp
	$(CC) -c $(INC) -o $@ $< $(CFLAGS) 

$(TEST_EXE): $(TEST_OBJS) 
	$(CC) $(CFLAGS) $(TEST_OBJS) -o $(TEST_EXE)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o build/release/$(EXE)
	rm -f $(DEBUG_ODIR)/*.o build/debug/$(DEBUG_EXE)
	rm -f $(TEST_ODIR)/*.o $(TEST_EXE)