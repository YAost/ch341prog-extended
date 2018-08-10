CFLAGS=-std=gnu99 -Wall
ch341prog: main.c src/ch341a.c include/ch341a.h src/ch341a_ext.c include/ch341a_ext.h
	gcc $(CFLAGS) src/ch341a.c src/ch341a_ext.c main.c -o ch341example  -lusb-1.0
clean:
	rm *.o ch341example -f
.PHONY: clean
