CXX = g++
CXXFLAGS = -Wall -O2 -Wno-reorder -I. -I./headers -I./stb
LDFLAGS = -lGL -lGLU -lglut

TARGET = scene_viewer
TEST_TARGET = tests/collision/collision_suite
SRCDIR = source

OBJS = $(SRCDIR)/main.o $(SRCDIR)/scene_loader.o $(SRCDIR)/manual_shapes.o $(SRCDIR)/textures.o \
	$(SRCDIR)/textures_path.o $(SRCDIR)/figures.o $(SRCDIR)/animation.o $(SRCDIR)/object_factory.o \
	$(SRCDIR)/transform_wrapper.o $(SRCDIR)/collision_mesh.o $(SRCDIR)/collision_repr.o \
	$(SRCDIR)/render_material.o $(SRCDIR)/fourd_collision.o $(SRCDIR)/fourd_math.o \
	$(SRCDIR)/fourd_figure.o

all: $(TARGET)

test:
	$(MAKE) -C tests/collision test

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
