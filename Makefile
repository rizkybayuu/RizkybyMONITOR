CXX      = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra
LDFLAGS  = -L/usr/lib \
           -l:libgtk-3.so.0 \
           -l:libwebkit2gtk-4.1.so.0 \
           -l:libgobject-2.0.so.0 \
           -l:libglib-2.0.so.0 \
           -l:libgdk-3.so.0 \
           -lpthread

# Output binary is named rizkybymonitor_linux to make the platform explicit
TARGET = rizkybymonitor_linux
SRC    = src/main_linux.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
