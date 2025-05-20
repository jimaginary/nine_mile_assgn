CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra `sdl2-config --cflags`
TARGETS = event_replay imbalance_logger

all: $(TARGETS)

event_replay: event_replay.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

imbalance_logger: imbalance_logger.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)
