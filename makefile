a : main.o
	g++ -o a main.o 
main.o : main.c
	g++ -c main.c
clean :
	rm -f *.o *~ a