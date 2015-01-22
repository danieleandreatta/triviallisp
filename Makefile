OBJ=lisp.o reader.o print.o logging.o

CFLAGS=-Wall

lisp: $(OBJ)
	gcc $(CFLAGS) -o $@ $(OBJ)


clean:
	rm $(OBJ)

.c.o:
	gcc $(CFLAGS) -c $< -o $@
