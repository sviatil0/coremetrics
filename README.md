# SP26_Team04

# main.cpp
## Description
main.cpp is a demonstration program that utilizes the Screen and GUIFile classes to load, display, and save GUI elements.


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

### splitString(const std::string &str, const std::string &delim)
This method takes in a string to split and a set of delimiters as a string to split it by. A string vector stores the substrings. Once the find method does not find anymore delimiters, the method returns the populated vector with the separated tokens.

### void readFile(std::string fileName)
Parses a specified XML file into GUI elements. The method clears all internal containers before parsing to ensure no data overlap.

### void writeFile(std::string fileName)
Writes currently staged elements to an XML file. The output follows a strict schema where nested tags are indented and value tags (like <x>) are on a single line for human readability.

# GUIElement
## Description
An abstract base class (ABC) that serves as the foundation for all renderable UI components. It enforces a polymorphic interface, ensuring that any UI component can be drawn using a provided `Screen` reference without the caller needing to know the specific type of the element.

## Methods
### virtual void draw(Screen& screen) = 0
A pure virtual method that subclasses must implement. By receiving a `Screen` reference as a parameter, the element remains decoupled from the specific rendering target, allowing for better memory efficiency and flexibility.

# Label
## Description
A UI component responsible for managing and displaying text strings. It currently utilizes a "placeholder" system that renders a proportional box for each character to verify layout, spacing, and color before a full font engine is integrated.

## Methods
### void draw(Screen& screen) override
Iterates through the stored string and calculates the screen coordinates for each character, rendering a filled box via the screen's `drawBox` method to represent the text footprint.

### std::string getText() const
Returns the string content currently assigned to the label.

# Selection
## Description
A stateful checkbox component used for user input. It manages a boolean selection state and provides a multi-layered visual representation including a border, a background, and a conditional "check" mark.

## Methods
### void draw(Screen& screen) override
Renders the checkbox components. It draws a white border and dark background. If the internal state is set to true, it renders a green internal mark to indicate the "selected" status.

### void toggle()
Inverts the current selection state (flips between true and false).

### bool isSelected() const
Returns the current boolean value of the checkbox state.

# Image
## Description
A component designed to render bitmap (BMP) assets. Following a "reductionist" approach, this class avoids high-level SDL blitting functions and instead manually iterates through raw pixel buffers to plot images coordinate-by-coordinate.

## Methods
### void draw(Screen& screen) override
Loads a BMP file into a temporary buffer, converts it to a standard RGBA8888 format, and executes a nested loop to plot every non-transparent pixel to the screen using `drawPixel`.

### std::string getFilePath() const
Returns the file path of the image asset associated with this object.