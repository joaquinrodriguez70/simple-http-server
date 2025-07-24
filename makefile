default :server

filelog.o: filelog.c
	gcc -c filelog.c -o filelog.o

server.o: server.c
	gcc -c server.c -o server.o

server: server.o filelog.o
	gcc server.o filelog.o  -o server

