CXX = g++
INCLUDES = -I../headers
CXXFLAGS = -Wall -O2 -fPIC -shared $(INCLUDES)

EXAMPLES = orbit.so linear_spin.so

all: $(EXAMPLES)

orbit.so: examples/orbit.cpp ../headers/object_script_api.h
	$(CXX) $(CXXFLAGS) -o $@ $<

linear_spin.so: examples/linear_spin.cpp ../headers/object_script_api.h
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(EXAMPLES)

.PHONY: all clean
