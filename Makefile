a.out: bc_rabin.c bc_rabin.h db.h Md5.c test.c
	gcc -O0 -g bc_rabin.c Md5.c test.c
bc_rabin.o: bc_rabin.c
	gcc -c -g -O0 bc_rabin.c

clean:
	rm -rf *.o a.out *.gch
