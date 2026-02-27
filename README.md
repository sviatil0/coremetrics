# SP26_Team04

# main.cpp
## Description
main.cpp is a demonstration program that utilizes the Screen and GUIFile classes to load, display, and save GUI elements.

# GUIFile
## Description
This class interacts with xml files to store and write information about how to create a GUI. It utilizes GUIElements.hpp, which defines in structs the elements (Line, Box, and Point) that describe the GUI. The lines, points, and boxes stored in the class are defined by vector containers in order to allow for a variable size and appending via push_back().

## Methods
### splitString(const std::string &str, const std::string &delim)
This method takes in a string to split and a set of delimiters as a string to split it by. A string vector stores the substrings, found using the find_first_of() string method which continuously finds the indices of the delimiters (places where the string could be split). Once the find method does not find anymore delimiters, the method returns the populated vector with the separated tokens.

# Matrix
## Description
Class description

## Methods
### return identifier(parameter list)
Description of method

etc..

# vec2
## Description
Class description

## Methods
### return identifier(parameter list)
Description of method

etc..

# vec3
## Description
Class description

## Methods
### return identifier(parameter list)
Description of methods

etc..

# Screen
## Description
The Screen class provides an interface for rendering geometric primitives onto an SDL surface.

## Methods
### void drawPixel(ivec2 pos, vec3 color)
Colors a single pixel at the specified integer coordinate.

### void drawLine(ivec2 start, ivec2 end, vec3 color)
Renders a line between two points. This method handles horizontal, vertical, and diagonal orientations.

### void drawBox(ivec2 minPos, ivec2 maxPos, vec3 color)
Renders a rectangle between the specified minimum and maximum corners.

### void blitTo(SDL_Surface* surface)
Copies the internal Screen buffer to the provided SDL surface for display.

# GUIFile
## Description
The GUIFile class manages the loading, staging, and saving of GUI layout elements. It acts as the bridge between the internal data structures and the external XML file format.

## Containers
* **std::vector<Point>**: Stores individual point elements parsed from XML or staged for saving.
* **std::vector<Line>**: Stores line elements, including start/end positions and color data.
* **std::vector<Box>**: Stores box elements, including min/max bounds and color data.
* **Selection Logic**: Vectors provide contiguous memory for fast iteration during rendering and writing. To optimize for mobile use and memory efficiency, all containers are cleared before loading new data from a file.

## Methods
### void setPoint(Point point)
Adds a Point object to the internal container, staging it for a file write operation.

### void setLine(Line line)
Adds a Line object to the internal container, staging it for a file write operation.

### void setBox(Box box)
Adds a Box object to the internal container, staging it for a file write operation.

### std::vector<Point> getPoints()
Returns a copy of the internal points vector. Returning by value ensures the class's internal data cannot be modified by the caller.

### std::vector<Line> getLines()
Returns a copy of the internal lines vector to provide read-only access to staged data.

### std::vector<Box> getBoxes()
Returns a copy of the internal boxes vector to provide read-only access to staged data.

### void readFile(std::string fileName)
Parses a specified XML file into GUI elements. The method clears all internal containers before parsing to ensure no data overlap.

### void writeFile(std::string fileName)
Writes currently staged elements to an XML file. The output follows a strict schema where nested tags are indented and value tags (like <x>) are on a single line for human readability.
