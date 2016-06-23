### Makefile for AUDIO ###

# Change to desired executable name
EXECUTABLE = audio
# Change to desired compiler
CXX = g++
UNAME_S := $(shell uname -s)

TESTSOURCES = $(wildcard *test*.cpp)
HEADERS = $(wildcard *.h)
SOURCES = *.cpp #$(wildcard *.cpp)
SOURCES := $(filter-out $(TESTSOURCES), $(SOURCES))
OBJECTS = $(SOURCES:%.cpp=%.o)
DLIBS = $(wildcard *.dylib)
ALIBS = $(wildcard *.a)

LIBS = ""

ifeq ($(OS),Windows_NT)
    OS_detected := Windows
else
    OS_detected := $(shell uname -s)
endif

ifeq ($(OS_detected),Darwin)
	LIBS = $(DLIBS)
else
 	ifeq ($(OS_detected),Linux)	
		LIBS = -lportaudio
	endif
endif

#Default Flags
CXXFLAGS = -std=c++11 -Wall -Wvla -Wconversion -Wextra -pedantic

# make release - will compile "all" with $(CXXFLAGS) and the -O3 flag
#        also defines NDEBUG so that asserts will not check
release: CXXFLAGS += -O3 -DNDEBUG
release: all

# make debug - will compile "all" with $(CXXFLAGS) and the -g flag
#              also defines DEBUG so that "#ifdef DEBUG /*...*/ #endif" works
debug: CXXFLAGS += -g3 -DDEBUG
debug: clean all

# highest target; sews together all objects into executable
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
ifeq ($(EXECUTABLE), executable)
	@echo Using default a.out.
	$(CXX) $(CXXFLAGS) $(OBJECTS)
else
	@echo $(HEADERS) $(SOURCES) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LIBS) -pthread -o $(EXECUTABLE)
endif

$(OBJECTS): $(SOURCES) $(HEADERS)
	@echo $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -c

# make clean - remove .o files, executables, tarball
clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(TESTS)
