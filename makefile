all: ./ds ./peer

./peer: ./peer.o ./util/msg.o ./util/util_c.o ./util/neighbors.o ./util/util.o
	gcc -Wall ./peer.o ./util/msg.o ./util/util_c.o ./util/neighbors.o ./util/util.o -o ./peer

./ds: ./ds.o ./util/util_s.o ./util/util.o ./util/peer_file.o ./util/msg.o  ./util/neighbors.o
	gcc -Wall ./ds.o ./util/util_s.o ./util/util.o ./util/peer_file.o ./util/msg.o ./util/neighbors.o -o ./ds

./peer.o: ./peer.c ./util/msg.h ./util/util_c.h ./util/const.h
	gcc -Wall -c ./peer.c -o ./peer.o

./ds.o: ./ds.c ./util/msg.h ./util/peer_file.h ./util/util_s.h ./util/const.h
	gcc -Wall -c ./ds.c -o ./ds.o

./util/peer_file.o: ./util/peer_file.c ./util/peer_file.h ./util/const.h ./util/neighbors.o
	gcc -Wall -c ./util/peer_file.c ./util/neighbors.o -o ./util/peer_file.o

./util/util.o: ./util/util.c ./util/util.h 
	gcc -Wall -c ./util/util.c -o ./util/util.o

./util/msg.o: ./util/msg.c ./util/msg.h ./util/const.h
	gcc -Wall -c ./util/msg.c -o ./util/msg.o

./util/util_c.o: ./util/util_c.c ./util/util_c.h ./util/msg.h ./util/const.h
	gcc -Wall -c ./util/util_c.c -o ./util/util_c.o

./util/util_s.o: ./util/util_s.c ./util/util_s.h ./util/const.h
	gcc -Wall -c ./util/util_s.c -o ./util/util_s.o

clean:
	-rm ./*.o ./ds ./peer ./util/*.o
	-mv ./ds_dir/2021* ./logs
	-rm ./ds_dir/*.txt
	-mv ./peer_dir/*.txt ./logs