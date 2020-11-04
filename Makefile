# C++ compiler
CC = g++
# C++ compiler flags
CFLAGS = -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow
# Target executable name
TARGET = httpserver

all: $(TARGET)

# Used to build executable from .o file(s)
$(TARGET): $(TARGET).o
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).o

# Used to build .o file(s) from .c files(s)
$(TARGET).o: $(TARGET).cpp
	$(CC) $(CFLAGS) -c $(TARGET).cpp

# clean built artifacts
clean:
	rm -f $(TARGET) $(TARGET).o
