CXX = g++
CXXFLAGS = -Wall -g
TARGET = wsl_server
SRCS = wsl_server.cpp read_mesh.cpp read_result.cpp
OBJS = $(SRCS:.cpp=.o)
INCLUDE_DIRS = .
LDFLAGS = -lz 

# Default rule
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile
%.o: %.cpp
	$(CXX) -I$(INCLUDE_DIRS) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)