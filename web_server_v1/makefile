AtomServer: http_connect.o log.h http_connect.h threadpool.h mutex_lock.h atom_server.cpp
	g++ atom_server.cpp http_connect.o log.h http_connect.h threadpool.h mutex_lock.h -o AtomServer -O3 -pthread

driver: driver.cpp mutex_lock.h threadpool.h log.h
	g++ driver.cpp mutex_lock.h threadpool.h log.h -o driver -O0 -pthread

http_connect.o : log.h mutex_lock.h threadpool.h http_connect.h http_connect.cpp
	g++ http_connect.h http_connect.cpp log.h mutex_lock.h threadpool.h -c -pthread -O3

.PHONY : clean
clean :
	-rm driver AtomServer $(objects) *.h.gch *.o