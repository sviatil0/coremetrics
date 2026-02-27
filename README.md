# SP26_Team04

# main.cpp
## Description
main.cpp is a demonstration program

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


### readFile(std::string fileName)
This method takes in a std::string file name to open and parse, assuming it is an xml file. If it exists, the class reads in the file using the splitString() method to divide the content where tags are found (indicated by < and >). Then, each token is processed using a very long switch case statement to make sure the proper tokens are contained inside of its opening and closing tags. If the parser ever encounters an out of place token, it returns. If the file did not exist in the first place, no GUI elements are processed or stored and the method returns.

