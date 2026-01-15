CXX = g++
CXXFLAGS = -O3 -Wall

all: tcp_server tcp_client

tcp_server: tcp_server.cpp
	$(CXX) $(CXXFLAGS) -o tcp_server tcp_server.cpp

tcp_client: tcp_client.cpp
	$(CXX) $(CXXFLAGS) -o tcp_client tcp_client.cpp

clean:
	rm -f tcp_server tcp_client
