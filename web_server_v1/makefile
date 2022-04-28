driver: driver.cpp mutex_lock.h threadpool.h log.h
	g++ driver.cpp mutex_lock.h threadpool.h log.h -o driver -O0 -pthread

http_connect.o : log.h mutex_lock.h threadpool.h http_connect.h http_connect.cpp
	g++ http_connect.h http_connect.cpp log.h mutex_lock.h threadpool.h -c -pthread -O3

.PHONY : clean
clean :
	-rm driver $(objects) *.h.gch *.o