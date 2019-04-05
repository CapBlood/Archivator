CC = gcc-8
NAME = archive
FLAGS = -w -fsanitize=address
.PHONY: all clean run

all: archive

clean:
	rm *.o
	rm $(NAME)

run: all
	./$(NAME)

main.o: main.c zar.h
	$(CC) $(FLAGS) -c -o main.o main.c

zar.o: zar.c zar.h
	$(CC) $(FLAGS) -c -o zar.o zar.c

archive: main.o zar.o
	$(CC) $(FLAGS) -o $(NAME) main.o zar.o
