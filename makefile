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
	-rm -rf ./data/
	
	mkdir -p ./data/5001/n/entries/
	echo 101 > ./data/5001/n/entries/2021_07_28.txt
	echo 102 > ./data/5001/n/entries/2021_07_29.txt
	echo 103 > ./data/5001/n/entries/2021_07_30.txt
	echo 104 > ./data/5001/n/entries/2021_07_31.txt
	echo 105 > ./data/5001/n/entries/2021_08_01.txt
	echo 106 > ./data/5001/n/entries/2021_08_02.txt
	echo 107 > ./data/5001/n/entries/2021_08_03.txt
	
	mkdir -p ./data/5002/n/entries/
	echo 201 > ./data/5002/n/entries/2021_07_28.txt
	echo 202 > ./data/5002/n/entries/2021_07_29.txt
	echo 203 > ./data/5002/n/entries/2021_07_30.txt
	echo 204 > ./data/5002/n/entries/2021_07_31.txt
	echo 205 > ./data/5002/n/entries/2021_08_01.txt
	echo 206 > ./data/5002/n/entries/2021_08_02.txt
	echo 207 > ./data/5002/n/entries/2021_08_03.txt
	
	mkdir -p ./data/5003/n/entries/
	echo 301 > ./data/5003/n/entries/2021_07_28.txt
	echo 302 > ./data/5003/n/entries/2021_07_29.txt
	echo 303 > ./data/5003/n/entries/2021_07_30.txt
	echo 304 > ./data/5003/n/entries/2021_07_31.txt
	echo 305 > ./data/5003/n/entries/2021_08_01.txt
	echo 306 > ./data/5003/n/entries/2021_08_02.txt
	echo 307 > ./data/5003/n/entries/2021_08_03.txt
	
	mkdir -p ./data/5004/n/entries/
	echo 401 > ./data/5004/n/entries/2021_07_28.txt
	echo 402 > ./data/5004/n/entries/2021_07_29.txt
	echo 403 > ./data/5004/n/entries/2021_07_30.txt
	echo 404 > ./data/5004/n/entries/2021_07_31.txt
	echo 405 > ./data/5004/n/entries/2021_08_01.txt
	echo 406 > ./data/5004/n/entries/2021_08_02.txt
	echo 407 > ./data/5004/n/entries/2021_08_03.txt
	
	
