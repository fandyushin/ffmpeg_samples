TARGET := audio-normalization
PATH-PROJECT := $(shell pwd)
OBJECTS_DIRS := $(PATH-PROJECT)
CXX := g++ -std=gnu++11 -D__STDC_CONSTANT_MACROS
CXXFLAGS := -m64 -g -v
LDFLAGS += -L/usr/local/Cellar/ffmpeg/3.0.2/lib -lavdevice -lavformat -lavfilter -lavcodec -lswresample -lavutil
INCLUDES := -I/usr/local/Cellar/ffmpeg/3.0.2/include
OBJS := main.o

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ -c $< $(LDFLAGS)

clean:
	rm -rf $(OBJECTS_DIRS)/*.o $(OBJECTS_DIRS)/$(TARGET)
