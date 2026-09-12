CXX = g++
CXXFLAGS = -Wall -O2 -Wno-reorder -I. -I./headers -I./stb -MMD -MP
LDFLAGS = -lGL -lGLU -lglut -ldl

TARGET = scene_viewer
TEST_TARGET = tests/collision/collision_suite
SRCDIR = source

OBJS = $(SRCDIR)/main.o $(SRCDIR)/scene_loader.o $(SRCDIR)/manual_shapes.o $(SRCDIR)/textures.o \
	$(SRCDIR)/textures_path.o $(SRCDIR)/figures.o $(SRCDIR)/animation.o $(SRCDIR)/object_factory.o \
	$(SRCDIR)/transform_wrapper.o $(SRCDIR)/collision_mesh.o $(SRCDIR)/collision_repr.o \
	$(SRCDIR)/render_material.o $(SRCDIR)/fourd_collision.o $(SRCDIR)/fourd_math.o \
	$(SRCDIR)/fourd_figure.o $(SRCDIR)/object_script_host.o $(SRCDIR)/editable_mesh.o \
	$(SRCDIR)/engine_power.o

all: $(TARGET)

scripts:
	$(MAKE) -C scripts

test:
	$(MAKE) -C tests/collision test

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(TARGET)

# Пересобирать объектники при изменении заголовков (физика живёт в scene.h).
-include $(OBJS:.o=.d)

.PHONY: all clean scripts test
