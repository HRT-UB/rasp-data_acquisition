all: clean myCAN
test:
	$(CC) -o test test.c -L. -L.. -lpthread -lusbcan 
myCAN:
	$(CC) -o ./build/myCAN myCAN.c -L. -L.. -lpthread -lusbcan -lsqlite3
clean:
	rm -vf test myCAN
