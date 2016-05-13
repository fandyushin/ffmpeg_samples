TARGET := minimal-ffmpeg
PATH-PROJECT := $(shell pwd)
OBJECTS_DIRS := $(PATH-PROJECT)
CXX := g++ -std=gnu++0x -D__STDC_CONSTANT_MACROS
CXXFLAGS := -m64 -g -v
LDFLAGS += -L/opt/local/lib -lavformat -lavcodec -lavutil
INCLUDES := -I/opt/local/include
OBJS := main.o

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ -c $< $(LDFLAGS)

clean:
	rm -rf $(OBJECTS_DIRS)/*.o $(OBJECTS_DIRS)/minimal-ffmpeg
