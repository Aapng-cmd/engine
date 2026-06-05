CXX = g++
CXXFLAGS = -Wall -O2 -Wno-reorder -I. -I./headers -I./stb
LDFLAGS = -lGL -lGLU -lglut

TARGET = scene_viewer
SRCDIR = source

OBJS = $(SRCDIR)/main.o $(SRCDIR)/scene_loader.o $(SRCDIR)/manual_shapes.o $(SRCDIR)/textures.o \
	$(SRCDIR)/textures_path.o $(SRCDIR)/figures.o $(SRCDIR)/animation.o $(SRCDIR)/object_factory.o \
	$(SRCDIR)/transform_wrapper.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
