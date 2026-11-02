CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall

TARGET_MAIN = cinebook_ds
TARGET_TEST = test_ds

ifeq ($(OS),Windows_NT)
    EXT = .exe
    RM = del /Q /F
else
    EXT =
    RM = rm -f
endif

all: $(TARGET_MAIN)$(EXT) $(TARGET_TEST)$(EXT)

$(TARGET_MAIN)$(EXT): data_structures.cpp main.cpp data_structures.hpp
	$(CXX) $(CXXFLAGS) data_structures.cpp main.cpp -o $(TARGET_MAIN)$(EXT)

$(TARGET_TEST)$(EXT): data_structures.cpp test_ds.cpp data_structures.hpp
	$(CXX) $(CXXFLAGS) data_structures.cpp test_ds.cpp -o $(TARGET_TEST)$(EXT)

test: $(TARGET_TEST)$(EXT)
	./$(TARGET_TEST)$(EXT)

clean:
	$(RM) $(TARGET_MAIN)$(EXT) $(TARGET_TEST)$(EXT)

.PHONY: all test clean
