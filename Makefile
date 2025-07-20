CXX        := g++
CXXFLAGS   := -std=c++14 -I/usr/include/opencv4
LDFLAGS    := `pkg-config --libs opencv4`
TARGET     := capture

all: $(TARGET)

$(TARGET): capture.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o snapshot.jpg

.PHONY: all clean