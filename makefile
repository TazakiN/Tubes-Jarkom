# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -Wextra -std=c++11

# Source files
SRCS = $(wildcard *.cpp)

# Object files directory
OBJDIR = build

# Object files
OBJS = $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS))

# Executable name
EXEC = node

# Default target
all: $(EXEC)

# Link object files to create executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean up build files
clean:
	rm -rf $(OBJDIR) $(EXEC)

.PHONY: all clean