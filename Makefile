# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -std=c++17 -Ofast

DEF = #-DNO_PLOT

INC = -IC:/Graphviz-12.1.0-win64/include

LIB = -LC:/Graphviz-12.1.0-win64/lib -lcgraph -lcdt -lgvc

# Source files
SRCS = main.cpp

# Executable name
TARGET = main.exe

# Default target
all: $(SRCS)
	$(CC) $(CFLAGS) $(DEF) $(INC) $(LIB) $(SRCS) -o $(TARGET) -v

# Clean rule
clean:
	del $(TARGET)