TARGET := ../originc
LIBS := 
CC := g++
CFLAGS := -std=c++17 -g -Wall -Wno-unused-variable

.PHONY: clean all default

default: $(TARGET)
all: default

OBJECTS := $(patsubst %.cpp, %.o, $(wildcard *.cpp) $(wildcard */*.cpp))
HEADERS := $(wildcard *.h) $(wildcard */*.h)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall -o $@ $(LIBS)

clean:
	-rm -f *.o
	-rm -f */*.o
	-rm -f $(TARGET)