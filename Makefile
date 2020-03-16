CC=gcc
CFLAGS=-O3 -Wall -fpic -std=c99

all: vm host

vm: socketiv-vm.o stub.o intr.o
	$(CC) $(CFLAGS) -shared -o socketiv-vm.so socketiv-vm.o stub.o intr.o
  
host: socketiv-host.o stub.o intr.o
	$(CC) $(CFLAGS) -shared -o socketiv-host.so socketiv-host.o stub.o intr.o

socketiv-vm.o: src/socketiv.c
	$(CC) $(CFLAGS) -c -o socketiv-vm.o src/socketiv.c
  
socketiv-host.o: src/socketiv.c
	$(CC) $(CFLAGS) -c -o socketiv-host.o src/socketiv.c -DSOCKETIV_IN_HOST

stub.o: src/stub.c
	$(CC) $(CFLAGS) -c -o stub.o src/stub.c

intr.o: src/intr.c
	$(CC) $(CFLAGS) -c -o intr.o src/intr.c

clean:
	rm -f *.so *.o
