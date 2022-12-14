CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=duejiewan
#CLASSES=SUA_LIB_COMUM
all: web-server web-client

web-server: 
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

web-client:
	$(CXX) -o $@ $^ $(CXXFLAGS) $@.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM web-server web-client *.tar.gz
tarball: clean
	tar -cvf $(USERID).tar.gz *
