CXX = g++
# CXXFLAGS = -std=c++23 -Wall -I ./include # MacOS, STEFAN
# LDFLAGS = -F/Library/Frameworks -framework sdl3  -Wl,-rpath,/Library/Frameworks  # MacOS, STEFAN
CXXFLAGS = -std=c++23 -Wall -I ./include -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 # Soleksii
LDFLAGS = -F/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 -framework SDL3 -Wl,-rpath,/Library/Frameworks/SDL3.xcframework/macos-arm64_x86_64 # Soleksii
# CXXFLAGS = -std=c++23 -Wall -F/Library/Frameworks -I ./include -framework sdl3  -Wl,-rpath,/Library/Frameworks  # Martin (TODO)
# CXXFLAGS = -std=c++17 -Wall -F/Library/Frameworks -I./include # Martin
# LDFLAGS = -F/Library/Frameworks -framework SDL3 # Martin
# CXXFLAGS = -std=c++17 -Wall -I./include -I$(HOME)/libs/SDL/include # Alicia
# LDFLAGS = -L$(HOME)/libs/SDL/build -lSDL3 -Wl,-rpath,$(HOME)/libs/SDL/build # also Alicia

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
               $(TESTDIR)/GUIElementTest.cpp \
               $(SRCDIR)/screen.cpp \
               $(SRCDIR)/matrix.cpp \
               $(SRCDIR)/GUIFile.cpp \
               $(SRCDIR)/Point.cpp \
               $(SRCDIR)/Line.cpp \
               $(SRCDIR)/Box.cpp \
               $(SRCDIR)/GUIElementFactory.cpp \
               $(SRCDIR)/image.cpp \
               $(SRCDIR)/label.cpp \
               $(SRCDIR)/selection.cpp \
               $(SRCDIR)/button.cpp
TEST_OBJECTS = $(OBJDIR)/tests.o \
               $(OBJDIR)/linearTest.o \
               $(OBJDIR)/screenTest.o \
               $(OBJDIR)/guiFileTest.o \
               $(OBJDIR)/GUIElementTest.o \
               $(OBJDIR)/TreeTest.o \
               $(OBJDIR)/LayoutManagerTest.o \
               $(OBJDIR)/screen.o \
               $(OBJDIR)/matrix.o \
               $(OBJDIR)/GUIFile.o \
               $(OBJDIR)/GUIElements.o \
               $(OBJDIR)/GUIElementFactory.o \
               $(OBJDIR)/image.o \
               $(OBJDIR)/label.o \
               $(OBJDIR)/selection.o \
               $(OBJDIR)/button.o \
               $(OBJDIR)/Layout.o \
               $(OBJDIR)/LayoutManager.o \
               $(OBJDIR)/Event.o \
               $(OBJDIR)/ClickEvent.o \
               $(OBJDIR)/ShowEvent.o \
               $(OBJDIR)/SoundEvent.o \
               $(OBJDIR)/EventManager.o \
               $(OBJDIR)/SoundPlayer.o
HEADERS = $(INCDIR)/linear.hpp $(INCDIR)/screen.hpp $(INCDIR)/linearTest.hpp $(INCDIR)/screenTest.hpp $(INCDIR)/guiFileTest.hpp

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

$(OBJDIR)/GUIElementTest.o: $(TESTDIR)/GUIElementTest.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/screen.o: $(SRCDIR)/screen.cpp $(INCDIR)/screen.hpp $(INCDIR)/linear.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/matrix.o: $(SRCDIR)/matrix.cpp $(INCDIR)/matrix.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/GUIFile.o: $(SRCDIR)/GUIFile.cpp $(INCDIR)/GUIFile.hpp $(INCDIR)/GUIElements.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/GUIElements.o: $(SRCDIR)/GUIElements.cpp $(INCDIR)/GUIElements.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/GUIElementFactory.o: $(SRCDIR)/GUIElementFactory.cpp $(INCDIR)/GUIElementFactory.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/label.o: $(SRCDIR)/label.cpp $(INCDIR)/label.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/selection.o: $(SRCDIR)/selection.cpp $(INCDIR)/selection.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/image.o: $(SRCDIR)/image.cpp $(INCDIR)/image.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/button.o: $(SRCDIR)/button.cpp $(INCDIR)/button.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/Layout.o: $(SRCDIR)/Layout.cpp $(INCDIR)/Layout.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/LayoutManager.o: $(SRCDIR)/LayoutManager.cpp $(INCDIR)/LayoutManager.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/TreeTest.o: $(TESTDIR)/TreeTest.cpp $(INCDIR)/TreeTest.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/LayoutManagerTest.o: $(TESTDIR)/LayoutManagerTest.cpp $(INCDIR)/LayoutManagerTest.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)/Event.o: $(SRCDIR)/Event.cpp $(INCDIR)/Event.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/ClickEvent.o: $(SRCDIR)/ClickEvent.cpp $(INCDIR)/ClickEvent.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/ShowEvent.o: $(SRCDIR)/ShowEvent.cpp $(INCDIR)/ShowEvent.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/SoundEvent.o: $(SRCDIR)/SoundEvent.cpp $(INCDIR)/SoundEvent.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/EventManager.o: $(SRCDIR)/EventManager.cpp $(INCDIR)/EventManager.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/SoundPlayer.o: $(SRCDIR)/SoundPlayer.cpp $(INCDIR)/SoundPlayer.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

$(DEMO_TARGET): $(OBJDIR)/main.o $(OBJDIR)/screen.o $(OBJDIR)/matrix.o $(OBJDIR)/button.o $(OBJDIR)/Layout.o $(OBJDIR)/LayoutManager.o $(OBJDIR)/Event.o $(OBJDIR)/ClickEvent.o $(OBJDIR)/ShowEvent.o $(OBJDIR)/SoundEvent.o $(OBJDIR)/EventManager.o $(OBJDIR)/SoundPlayer.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/main.o: main.cpp $(INCDIR)/screen.hpp $(INCDIR)/linear.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)