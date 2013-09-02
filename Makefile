query: query.o
	g++ -g -L /usr/local/mysql/lib -lmysqlclient -lhiredis query.o -o query
query.o: query.cpp
	g++ -g -I /usr/local/include/hiredis \
		-I /usr/local/mysql/include \
		-c query.cpp
clean:
	rm -f *.o
	rm -f query
