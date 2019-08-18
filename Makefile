## -------------------------------------------------------------------- *
##
## Copyright 2012 Angelo Masci
## 
## Permission is hereby granted, free of charge, to any person obtaining a
## copy of this software and associated documentation files (the 
## "Software"), to deal in the Software without restriction, including 
## without limitation the rights to use, copy, modify, merge, publish, 
## distribute, sublicense, and/or sell copies of the Software, and to permit 
## persons to whom the Software is furnished to do so, subject to the 
## following conditions:
## 
## The above copyright notice and this permission notice shall be included
## in all copies or substantial portions of the Software.
## 
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
## OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
## MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
## IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
## CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
## OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR 
## THE USE OR OTHER DEALINGS IN THE SOFTWARE.
## 
## --------------------------------------------------------------------

CC=gcc
CFLAGS=-I. -I./extras -O3 -Wall -Wextra -fomit-frame-pointer -march=native -mtune=native -std=c89
C99CFLAGS=-I. -I./extras -O3 -Wall -Wextra -fomit-frame-pointer -march=native -mtune=native -D_GNU_SOURCE -std=c99 

all: ambencode examples/example1 examples/example3 

ambencode.o: ambencode.c ambencode.h
	$(CC) -c -o ambencode.o ambencode.c $(CFLAGS)

extras/ambencode_util.o: extras/ambencode_util.c extras/ambencode_util.h  ambencode.h
	$(CC) -c -o extras/ambencode_util.o extras/ambencode_util.c $(CFLAGS)

extras/ambencode_dump.o: extras/ambencode_dump.c extras/ambencode_dump.h ambencode.h
	$(CC) -c -o extras/ambencode_dump.o extras/ambencode_dump.c $(CFLAGS)

extras/ambencode_file.o: extras/ambencode_file.c extras/ambencode_file.h ambencode.h
	$(CC) -c -o extras/ambencode_file.o extras/ambencode_file.c $(CFLAGS)

extras/ambencode_query.o: extras/ambencode_query.c extras/ambencode_query.h extras/ambencode_util.h ambencode.h
	$(CC) -c -o extras/ambencode_query.o extras/ambencode_query.c $(CFLAGS)

extras/ambencode_main.o: extras/ambencode_main.c ambencode.h extras/ambencode_file.h extras/ambencode_dump.h extras/ambencode_query.h extras/ambencode_util.h
	$(CC) -c -o extras/ambencode_main.o extras/ambencode_main.c $(C99CFLAGS)

ambencode: ambencode.o extras/ambencode_util.o extras/ambencode_dump.o extras/ambencode_file.o extras/ambencode_query.o extras/ambencode_main.o
	$(CC) -o ambencode ambencode.o extras/ambencode_util.o extras/ambencode_dump.o extras/ambencode_file.o extras/ambencode_query.o extras/ambencode_main.o $(CFLAGS)

examples/example1.o: ambencode.o examples/example1.c
	$(CC) -c -o examples/example1.o examples/example1.c $(CFLAGS)

examples/example1: ambencode.o examples/example1.o extras/ambencode_dump.o
	$(CC) -o examples/example1 ambencode.o examples/example1.o extras/ambencode_dump.o $(CFLAGS)

examples/example3.o: ambencode.o examples/example3.c
	$(CC) -c -o examples/example3.o examples/example3.c $(CFLAGS)

examples/example3: ambencode.o examples/example3.o extras/ambencode_dump.o extras/ambencode_query.o extras/ambencode_util.o
	$(CC) -o examples/example3 ambencode.o examples/example3.o extras/ambencode_dump.o extras/ambencode_query.o extras/ambencode_util.o $(CFLAGS)

.PHONY: clean

clean:
	rm -f ambencode ambencode.o extras/ambencode_util.o extras/ambencode_dump.o extras/ambencode_file.o \
              extras/ambencode_query.o extras/ambencode_main.o examples/example1 \
              examples/example1.o examples/example3 examples/example3.o 

## --------------------------------------------------------------------
## --------------------------------------------------------------------
