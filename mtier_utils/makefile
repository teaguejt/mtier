CC=gcc

all:
	$(CC) -Wall -c vaddr2paddr.c
	ar -cvq libpaddr.a vaddr2paddr.o
	#cp libpaddr.a /usr/local/bin
	$(CC) matmult.c -o matmult -L/usr/lib64/ -lnuma ./libpaddr.a
	$(CC) mtier.c   -o mtier   -L/usr/lib64/ -lnuma ./libpaddr.a
	$(CC) mtier2.c  -c					-lnuma
	$(CC) mtier2.o vaddr2paddr.o -lnuma -o mtier2
	$(CC) mtier3.c  -c					-lnuma
	$(CC) mtier3.o vaddr2paddr.o -lnuma -o mtier3
	$(CC) numavis.c -o numavis  -lnuma ./libpaddr.a
	$(CC) memvis.c  ./libpaddr.a -o memvis   -lnuma 
	cp libpaddr.a /usr/local/bin
	cp mtier* /usr/local/bin
