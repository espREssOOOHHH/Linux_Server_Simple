# args
ver ?=release

ifeq ($(ver),debug)
CXXFLAGS =-g -O0 -Wall
else ifeq ($(ver),release)
CXXFLAGS =-O3
else
CXXFLAGS =-O2
endif

AtomServer: http_connect.o log.h http_connect.h threadpool.h mutex_lock.h atom_server.cpp
	g++ $(CXXFLAGS) atom_server.cpp http_connect.o log.h http_connect.h threadpool.h mutex_lock.h -o AtomServer -pthread

driver: mutex_lock.h threadpool.h log.h
	g++ $(CXXFLAGS) driver.cpp mutex_lock.h threadpool.h log.h -o driver -pthread

http_connect.o : log.h mutex_lock.h threadpool.h
	g++ $(CXXFLAGS) http_connect.h http_connect.cpp log.h mutex_lock.h threadpool.h -c -pthread

.PHONY : clean
clean :
	-rm driver AtomServer $(objects) *.h.gch *.o