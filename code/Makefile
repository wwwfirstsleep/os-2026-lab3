CXX = c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic
scheduler: scheduler.cpp
	$(CXX) $(CXXFLAGS) scheduler.cpp -o scheduler
test: scheduler
	./scheduler --test
clean:
	rm -f scheduler
.PHONY: test clean
