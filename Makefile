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
BENCHDIR = bench
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Default install prefix for the manpage. Homebrew sets DESTDIR + prefix in
# its formula; distro packagers can override MANPATH on the command line
# (command-line assignments win over the makefile default below). Using
# plain `=` rather than `?=` so the user's shell MANPATH env var, which is
# a colon-separated search list and not a single install dir, does not
# poison the install location.
PREFIX  ?= /usr/local
MANPATH  = $(PREFIX)/share/man

DEMO_TARGET = $(BINDIR)/demo
TEST_TARGET = $(BINDIR)/tests
BENCH_TARGET = $(BINDIR)/bench
COREMETRICS_TARGET = $(BINDIR)/coremetrics

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
TEST_SOURCES = $(wildcard $(TESTDIR)/*.cpp)
BENCH_SOURCES = $(wildcard $(BENCHDIR)/*.cpp)

OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.cpp=$(OBJDIR)/%.o)
BENCH_OBJECTS = $(BENCH_SOURCES:$(BENCHDIR)/%.cpp=$(OBJDIR)/%.o)

.PHONY: all demo test bench coremetrics directories clean asan ubsan install-man

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

bench: directories $(BENCH_TARGET)
	./$(BENCH_TARGET)

coremetrics: directories $(COREMETRICS_TARGET)
	./$(COREMETRICS_TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(DEMO_TARGET): $(OBJDIR)/main.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

$(TEST_TARGET): $(TEST_OBJECTS) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

$(BENCH_TARGET): $(BENCH_OBJECTS) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

$(COREMETRICS_TARGET): $(OBJDIR)/coremetrics.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(EXTRA_LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(TESTDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(BENCHDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/coremetrics.o: coremetrics.cpp
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Install the section 1 manpage so `man coremetrics` works after `make
# install-man`. Homebrew's formula invokes this target with DESTDIR pointed
# at its keg; distro packagers can override MANPATH on the command line.
install-man:
	install -d $(DESTDIR)$(MANPATH)/man1
	install -m 0644 man/coremetrics.1 $(DESTDIR)$(MANPATH)/man1/coremetrics.1

clean:
	rm -rf $(OBJDIR) $(BINDIR)

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) $(BENCH_OBJECTS:.o=.d) $(OBJDIR)/main.d $(OBJDIR)/coremetrics.d
