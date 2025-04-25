CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

SRCS = main.cpp mph.cpp simple_hash.cpp elastic_hash.cpp funnel_hash.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = optimalhash

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
