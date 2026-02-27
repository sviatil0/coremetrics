CXX = g++
CXXFLAGS = -std=c++23 -Wall -I ./include# MacOS, STEFAN 
LDFLAGS = -F/Library/Frameworks -framework sdl3  -Wl,-rpath,/Library/Frameworks  
# CXXFLAGS = -std=c++23 -Wall -F/Library/Frameworks -I ./include -framework sdl3  -Wl,-rpath,/Library/Frameworks  # Martin (TODO)
# CXXFLAGS = -std=c++23 -Wall -F/Library/Frameworks -I ./include -framework sdl3  -Wl,-rpath,/Library/Frameworks  # Alicia (TODO)

SRCDIR = src
TESTDIR = tests
INCDIR = include
OBJDIR = obj
BINDIR = bin

DEMO_TARGET = $(BINDIR)/demo
TEST_TARGET = $(BINDIR)/tests

TEST_SOURCES = $(TESTDIR)/tests.cpp \
               $(TESTDIR)/linearTest.cpp \
               $(TESTDIR)/screenTest.cpp \
               $(SRCDIR)/screen.cpp
TEST_OBJECTS = $(OBJDIR)/tests.o \
               $(OBJDIR)/linearTest.o \
               $(OBJDIR)/screenTest.o \
               $(OBJDIR)/screen.o
HEADERS = $(INCDIR)/linear.hpp $(INCDIR)/screen.hpp $(INCDIR)/linearTest.hpp $(INCDIR)/screenTest.hpp

demo: directories $(DEMO_TARGET)
	./$(DEMO_TARGET)

test: directories $(TEST_TARGET)
	./$(TEST_TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/tests.o: $(TESTDIR)/tests.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/linearTest.o: $(TESTDIR)/linearTest.cpp $(HEADERS) 
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/screenTest.o: $(TESTDIR)/screenTest.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/screen.o: $(SRCDIR)/screen.cpp $(INCDIR)/screen.hpp $(INCDIR)/linear.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(DEMO_TARGET): $(OBJDIR)/main.o $(OBJDIR)/screen.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/main.o: main.cpp $(INCDIR)/screen.hpp $(INCDIR)/linear.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)