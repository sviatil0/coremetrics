CXX = g++
CXXFLAGS = -std=c++23 -Wall -I./include

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

TARGET = $(BINDIR)/linearTest

SOURCES = $(SRCDIR)/linearTest.cpp $(SRCDIR)/vec.cpp $(SRCDIR)/matrix.cpp

OBJECTS = $(OBJDIR)/linearTest.o $(OBJDIR)/vec.o $(OBJDIR)/matrix.o

HEADERS = $(INCDIR)/linear.hpp $(INCDIR)/vec.hpp $(INCDIR)/matrix.hpp

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

rebuild: clean all