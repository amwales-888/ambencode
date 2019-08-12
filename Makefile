## -------------------------------------------------------------------- *
##
## Copyright 2019 Angelo Masci
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
CFLAGS=-I. -I./extras -O3 -Wall -Wextra -pedantic-errors -fomit-frame-pointer -std=c89
DEPS=ambencode.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: ambencode examples/example1 examples/example3

ambencode: ambencode.o extras/ambencode_dump.o extras/ambencode_file.o extras/ambencode_main.o extras/ambencode_util.o extras/ambencode_query.o
	$(CC) -o $@ $^ $(C99CFLAGS)

examples/example1: ambencode.o examples/example1.o extras/ambencode_dump.o
	$(CC) -o $@ $^ $(CFLAGS)

examples/example3: ambencode.o examples/example3.o extras/ambencode_dump.o extras/ambencode_util.o extras/ambencode_query.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f ambencode ambencode.o extras/ambencode_dump.o extras/ambencode_file.o \
              extras/ambencode_util.o extras/ambencode_query.o \
              extras/ambencode_main.o examples/example1 examples/example3 \
              examples/example1.o 

## --------------------------------------------------------------------
## --------------------------------------------------------------------
