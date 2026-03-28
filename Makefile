CXX = g++
#CXXFLAGS = -std=c++23 -Wall -I ./include# MacOS, STEFAN 
#LDFLAGS = -F/Library/Frameworks -framework sdl3  -Wl,-rpath,/Library/Frameworks  
# CXXFLAGS = -std=c++23 -Wall -F/Library/Frameworks -I ./include -framework sdl3  -Wl,-rpath,/Library/Frameworks  # Martin (TODO)
# CXXFLAGS = -std=c++17 -Wall -F/Library/Frameworks -I./include # Martin
# LDFLAGS = -F/Library/Frameworks -framework SDL3 # Martin
CXXFLAGS = -std=c++17 -Wall -I./include -I$(HOME)/libs/SDL/include # Alicia
LDFLAGS = -L$(HOME)/libs/SDL/build -lSDL3 -Wl,-rpath,$(HOME)/libs/SDL/build # also Alicia

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
               $(TESTDIR)/guiFileTest.cpp \
               $(TESTDIR)/layoutTest.cpp \
               $(SRCDIR)/screen.cpp \
               $(SRCDIR)/matrix.cpp \
               $(SRCDIR)/GUIFile.cpp \
               $(SRCDIR)/Layout.cpp \
               $(SRCDIR)/selection.cpp
TEST_OBJECTS = $(OBJDIR)/tests.o \
               $(OBJDIR)/linearTest.o \
               $(OBJDIR)/screenTest.o \
               $(OBJDIR)/guiFileTest.o \
               $(OBJDIR)/layoutTest.o \
               $(OBJDIR)/screen.o \
               $(OBJDIR)/matrix.o \
               $(OBJDIR)/GUIFile.o \
               $(OBJDIR)/Layout.o \
               $(OBJDIR)/selection.o
HEADERS = $(INCDIR)/linear.hpp $(INCDIR)/screen.hpp $(INCDIR)/linearTest.hpp $(INCDIR)/screenTest.hpp $(INCDIR)/guiFileTest.hpp $(INCDIR)/layoutTest.hpp

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

$(OBJDIR)/guiFileTest.o: $(TESTDIR)/guiFileTest.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/screen.o: $(SRCDIR)/screen.cpp $(INCDIR)/screen.hpp $(INCDIR)/linear.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/matrix.o: $(SRCDIR)/matrix.cpp $(INCDIR)/matrix.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/GUIFile.o: $(SRCDIR)/GUIFile.cpp $(INCDIR)/GUIFile.hpp $(INCDIR)/GUIElements.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/layoutTest.o: $(TESTDIR)/layoutTest.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/Layout.o: $(SRCDIR)/Layout.cpp $(INCDIR)/Layout.hpp $(INCDIR)/GUIElement.hpp $(INCDIR)/linear.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/selection.o: $(SRCDIR)/selection.cpp $(INCDIR)/selection.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(DEMO_TARGET): $(OBJDIR)/main.o $(OBJDIR)/screen.o $(OBJDIR)/matrix.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/main.o: main.cpp $(INCDIR)/screen.hpp $(INCDIR)/linear.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)