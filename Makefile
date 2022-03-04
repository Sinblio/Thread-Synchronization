all: encrypt352

encrypt352: encrypt-module.o main.o
	gcc -o encrypt352 main.o encrypt-module.o -pthread -g

main.o: main.c
	gcc -c main.c

encrypt-module.o: encrypt-module.c encrypt-module.h
	gcc -c encrypt-module.c

clean:
	rm encrypt352 encrypt-module.o main.o