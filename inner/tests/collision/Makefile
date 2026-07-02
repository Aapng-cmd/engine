# Тесты коллизий: make test (все наборы)
INNER = ../..
CXX = g++
CXXFLAGS = -Wall -O2 -Wno-reorder -I$(INNER) -I$(INNER)/headers -I$(INNER)/stb
LDFLAGS = -lGL -lGLU -lglut

SRCDIR = $(INNER)/source
CORE = scene_loader.cpp manual_shapes.cpp textures.cpp textures_path.cpp figures.cpp \
	object_factory.cpp transform_wrapper.cpp collision_mesh.cpp collision_repr.cpp \
	render_material.cpp fourd_collision.cpp fourd_math.cpp fourd_figure.cpp
CORE_OBJS = $(CORE:%.cpp=$(SRCDIR)/%.o)

all: collision_suite pair_suite fourd_suite stress_suite k_axis_suite impulse_suite

test: all
	./collision_suite && ./pair_suite && ./fourd_suite && ./stress_suite && ./k_axis_suite && ./impulse_suite

collision_suite: $(CORE_OBJS) run_suite.o
	$(CXX) -o $@ $^ $(LDFLAGS)

pair_suite: $(CORE_OBJS) pair_suite.o
	$(CXX) -o $@ $^ $(LDFLAGS)

fourd_suite: fourd_suite.o $(SRCDIR)/fourd_collision.o $(SRCDIR)/fourd_math.o
	$(CXX) -o $@ $^ $(LDFLAGS)

stress_suite: $(CORE_OBJS) stress_suite.o
	$(CXX) -o $@ $^ $(LDFLAGS)

k_axis_suite: $(CORE_OBJS) k_axis_suite.o
	$(CXX) -o $@ $^ $(LDFLAGS)

impulse_suite: $(CORE_OBJS) impulse_suite.o
	$(CXX) -o $@ $^ $(LDFLAGS)

run_suite.o: run_suite.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

pair_suite.o: pair_suite.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

fourd_suite.o: fourd_suite.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

stress_suite.o: stress_suite.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

k_axis_suite.o: k_axis_suite.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

impulse_suite.o: impulse_suite.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f collision_suite pair_suite fourd_suite stress_suite k_axis_suite impulse_suite *.o

.PHONY: all test clean
