CXX = g++
CPPFLAGS = -I$(INCDIR) -MMD -MP
# CXXFLAGS = -std=c++23 -Wall -I ./include # MacOS, STEFAN
# LDFLAGS = -F/Library/Frameworks -framework sdl3  -Wl,-rpath,/Library/Frameworks  # MacOS, STEFAN
# CXXFLAGS = -std=c++23 -Wall -I ./include -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 # Soleksii
# LDFLAGS = -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 -framework SDL3 -Wl,-rpath,/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 # Soleksii
# CXXFLAGS = -std=c++23 -Wall -F/Library/Frameworks -I ./include -framework sdl3  -Wl,-rpath,/Library/Frameworks  # Martin (TODO)
# CXXFLAGS = -std=c++17 -Wall -F/Library/Frameworks -I./include # Martin
# LDFLAGS = -F/Library/Frameworks -framework SDL3 # Martin
# CXXFLAGS = -std=c++23 -Wall -I ./include -I/opt/homebrew/include # Soleksii (brew sdl3 + sdl3_ttf + sdl3_image)
# LDFLAGS = -L/opt/homebrew/lib -lSDL3 -lSDL3_ttf -lSDL3_image -Wl,-rpath,/opt/homebrew/lib -framework IOKit -framework CoreFoundation # Soleksii
CXXFLAGS = -std=c++17 -Wall -I./include -I$(HOME)/dev/sdl3/install/include # Alicia
LDFLAGS = -L$(HOME)/dev/sdl3/install/lib -lSDL3 -lSDL3_ttf -lSDL3_image -Wl,-rpath,$(HOME)/dev/sdl3/install/lib # also Alicia

SRCDIR = src
TESTDIR = tests
INCDIR = include
OBJDIR = obj
BINDIR = bin

DEMO_TARGET = $(BINDIR)/demo
TEST_TARGET = $(BINDIR)/tests
COREMETRICS_TARGET = $(BINDIR)/coremetrics

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
TEST_SOURCES = $(wildcard $(TESTDIR)/*.cpp)

OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.cpp=$(OBJDIR)/%.o)

demo: directories $(DEMO_TARGET)
	./$(DEMO_TARGET)

test: directories $(TEST_TARGET)
	./$(TEST_TARGET)

coremetrics: directories $(COREMETRICS_TARGET)
	./$(COREMETRICS_TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(DEMO_TARGET): $(OBJDIR)/main.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJECTS) $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(COREMETRICS_TARGET): $(OBJDIR)/coremetrics.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(TESTDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/coremetrics.o: coremetrics.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(OBJDIR)/main.d $(OBJDIR)/coremetrics.d
