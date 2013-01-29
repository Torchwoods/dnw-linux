all:dnw2 dnw
dnw2 : dnw2.c
	gcc -g -o $@ $^ -lusb
dnw : dnw.c
	gcc -g -o $@ $^
	
clean :
	rm -rf dnw dnw2 *.o

install : dnw dnw2
	cp $^ /usr/bin
