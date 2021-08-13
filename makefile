all: ./ds ./peer

./peer: ./peer.o ./util/msg.o ./util/util_c.o ./util/neighbors.o ./util/util.o ./util/date.o
	gcc -Wall ./peer.o ./util/msg.o ./util/util_c.o ./util/neighbors.o ./util/util.o ./util/date.o -o ./peer

./ds: ./ds.o ./util/util_s.o ./util/util.o ./util/peer_file.o ./util/msg.o  ./util/neighbors.o ./util/date.o
	gcc -Wall ./ds.o ./util/util_s.o ./util/util.o ./util/peer_file.o ./util/msg.o ./util/neighbors.o ./util/date.o -o ./ds

./peer.o: ./peer.c ./util/util_c.h 
	gcc -Wall -c ./peer.c -o ./peer.o

./ds.o: ./ds.c ./util/util_s.h
	gcc -Wall -c ./ds.c -o ./ds.o

./util/peer_file.o: ./util/peer_file.c ./util/peer_file.h ./util/util.h ./util/neighbors.h
	gcc -Wall -c ./util/peer_file.c -o ./util/peer_file.o

./util/util.o: ./util/util.c ./util/util.h 
	gcc -Wall -c ./util/util.c -o ./util/util.o

./util/msg.o: ./util/msg.c ./util/msg.h 
	gcc -Wall -c ./util/msg.c -o ./util/msg.o

./util/util_c.o: ./util/util_c.c ./util/util_c.h ./util/msg.h
	gcc -Wall -c ./util/util_c.c -o ./util/util_c.o

./util/util_s.o: ./util/util_s.c ./util/util_s.h 
	gcc -Wall -c ./util/util_s.c -o ./util/util_s.o

./util/date.o: ./util/date.c ./util/date.h
	gcc -Wall -c ./util/date.c -o ./util/date.o

clean:
	-rm -f ./*.o ./ds ./peer ./util/*.o 
	-rm -rf ./data/*/n/elabs/
