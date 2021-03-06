OBJS = src/client.o
CXX ?= g++
CXXFLAGS = -std=c++17 -fsanitize=address,undefined -g
LDFLAGS =
LIBS = -lboost_system -lboost_thread -pthread
TARGET = client

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS) $(CXXFLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

.PHONY: clean

clean:
	-rm $(OBJS)
	-rm $(TARGET)
