CC=g++
GDB=-ggdb -O0
TARGET=send_eth recv_eth
all: $(TARGET)

send_eth: send_eth.cc
	$(CC) $(GDB) -o $@ send_eth.cc

recv_eth: recv_eth.cc
	$(CC) $(GDB) -o $@ recv_eth.cc

clean: 
	rm -rf *.o send_eth recv_eth
    
