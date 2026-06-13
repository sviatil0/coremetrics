CXX = g++
CPPFLAGS = -I$(INCDIR) -MMD -MP
# CXXFLAGS = -std=c++23 -Wall -I ./include # MacOS, STEFAN
# LDFLAGS = -F/Library/Frameworks -framework sdl3  -Wl,-rpath,/Library/Frameworks  # MacOS, STEFAN
# CXXFLAGS = -std=c++23 -Wall -I ./include -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 # Soleksii
# LDFLAGS = -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 -framework SDL3 -Wl,-rpath,/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 # Soleksii
# CXXFLAGS = -std=c++23 -Wall -F/Library/Frameworks -I ./include -framework sdl3  -Wl,-rpath,/Library/Frameworks  # Martin (TODO)
# CXXFLAGS = -std=c++17 -Wall -F/Library/Frameworks -I./include # Martin
# LDFLAGS = -F/Library/Frameworks -framework SDL3 # Martin
CXXFLAGS = -std=c++23 -Wall -I ./include -I/opt/homebrew/include # Soleksii (brew sdl3 + sdl3_ttf + sdl3_image)
LDFLAGS = -L/opt/homebrew/lib -lSDL3 -lSDL3_ttf -lSDL3_image -Wl,-rpath,/opt/homebrew/lib -framework IOKit -framework CoreFoundation # Soleksii
# CXXFLAGS = -std=c++17 -Wall -I./include -I$(HOME)/dev/sdl3/install/include # Alicia
# LDFLAGS = -L$(HOME)/dev/sdl3/install/lib -lSDL3 -lSDL3_ttf -lSDL3_image -Wl,-rpath,$(HOME)/dev/sdl3/install/lib # also Alicia

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

.PHONY: all demo test coremetrics directories clean asan ubsan

# Rebuild the test suite under AddressSanitizer or UndefinedBehaviorSanitizer.
# These re-invoke `make test` with extra flags appended via EXTRA_CXXFLAGS /
# EXTRA_LDFLAGS so the regular CXXFLAGS line stays untouched. ASAN_FLAGS are
# also passed to the linker, which is required for the sanitizer runtime to
# be pulled in. The sanitizer output goes to stderr; tests still exit 0 on
# success and non-zero on any flagged behavior.

ASAN_FLAGS  = -fsanitize=address -fno-omit-frame-pointer -g
UBSAN_FLAGS = -fsanitize=undefined -fno-omit-frame-pointer -g

asan:
	$(MAKE) clean
	$(MAKE) test EXTRA_CXXFLAGS="$(ASAN_FLAGS)" EXTRA_LDFLAGS="$(ASAN_FLAGS)"

ubsan:
	$(MAKE) clean
	$(MAKE) test EXTRA_CXXFLAGS="$(UBSAN_FLAGS)" EXTRA_LDFLAGS="$(UBSAN_FLAGS)"

all: coremetrics

demo: directories $(DEMO_TARGET)
	./$(DEMO_TARGET)

test: directories $(TEST_TARGET)
	./$(TEST_TARGET)

coremetrics: directories $(COREMETRICS_TARGET)
	./$(COREMETRICS_TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(DEMO_TARGET): $(OBJDIR)/main.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

$(TEST_TARGET): $(TEST_OBJECTS) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

$(COREMETRICS_TARGET): $(OBJDIR)/coremetrics.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(TESTDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/coremetrics.o: coremetrics.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(OBJDIR)/main.d $(OBJDIR)/coremetrics.d
