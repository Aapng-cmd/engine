CXX = g++
CXXFLAGS = -Wall -O2 -Wno-reorder -I. -I./stb
LDFLAGS = -lGL -lGLU -lglut

TARGET = scene_viewer

OBJS = main.o scene_loader.o manual_shapes.o textures.o textures_path.o figures.o animation.o object_factory.o transform_wrapper.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

main.o: main.cpp scene_loader.h manual_shapes.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

scene_loader.o: scene_loader.cpp scene_loader.h manual_shapes.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

manual_shapes.o: manual_shapes.cpp manual_shapes.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

textures.o: textures.cpp textures.h textures_path.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

textures_path.o: textures_path.cpp textures_path.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

figures.o: figures.cpp figures.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

animation.o: animation.cpp animation.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

object_factory.o: object_factory.cpp object_factory.h manual_shapes.h transform_wrapper.h figures.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

transform_wrapper.o: transform_wrapper.cpp transform_wrapper.h figures.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
