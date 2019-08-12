/* -------------------------------------------------------------------- *

Copyright 2019 Angelo Masci

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sublicense, and/or sell copies of the Software, and to permit 
persons to whom the Software is furnished to do so, subject to the 
following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR 
THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 * -------------------------------------------------------------------- */

#include <stdio.h>

#include "ambencode.h"
#include "extras/ambencode_dump.h"

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

static void dump_spaces(int count);
static void dump_json(struct bhandle *bhandle, struct bobject *bobject,
		      int type, int depth, int pretty);
static void dump(struct bhandle *bhandle, struct bobject *bobject,
		 int type, int depth, int pretty);

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

#define MAX_SPACECOUNT 3

static char *spaces[] = {
  "","  ","    ","      ","        ","          ",
  "            ","              "
};

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void dump_spaces(int depth) {

  if (depth == 0) return;
  
  if (depth < MAX_SPACECOUNT) {
    printf(spaces[depth]);
  } else {    

    for (;;) {

      printf(spaces[MAX_SPACECOUNT-1]);
      
      depth -= MAX_SPACECOUNT-1;
      if (depth < MAX_SPACECOUNT) break;     
    }

    dump_spaces(depth);
  }
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void dump_json(struct bhandle *bhandle, struct bobject *bobject,
		      int type, int depth, int pretty) {

  char *sep   = "";
  char *nl    = "";
  
  if (pretty) {
    nl = "\n";
  }
  
  while (bobject) {

    printf(sep);
    if ((pretty) && (*sep != ':')) {
      dump_spaces(depth);
    }
    
    switch (BOBJECT_TYPE(bobject)) {

    case AMBENCODE_STRING:
      printf("\"%.*s\"", BOBJECT_STRING_LEN(bobject), BOBJECT_STRING_PTR(bhandle, bobject));
      break;
    case AMBENCODE_NUMBER:
      printf("%.*s", BOBJECT_STRING_LEN(bobject), BOBJECT_STRING_PTR(bhandle, bobject));
      break;
    case AMBENCODE_DICTIONARY:
      printf("{");
      if (pretty) printf(nl);      
      dump_json(bhandle, DICTIONARY_FIRST_KEY(bhandle, bobject), AMBENCODE_DICTIONARY, depth+1, pretty);
      if (pretty) printf(nl);
      if (pretty) dump_spaces(depth);
      printf("}");
      break;
    case AMBENCODE_LIST:
      printf("[");
      if (pretty) printf(nl);
      dump_json(bhandle, LIST_FIRST(bhandle, bobject), AMBENCODE_LIST, depth+1, pretty);
      printf(nl);
      if (pretty) dump_spaces(depth);
      printf("]");
      break;
    }

    if (depth == 0) {    
      bobject = (void *)0;
    } else {
      bobject = BOBJECT_NEXT(bhandle,bobject);
    }

    if ((type == AMBENCODE_DICTIONARY) &&
	((*sep == '\0') || (*sep == ','))) {      
      if (pretty) {
	sep = ": ";
      } else {
	sep = ":";
      }
    } else {
      if (pretty) {
	sep = ",\n";
      } else {
	sep = ",";
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void dump(struct bhandle *bhandle, struct bobject *bobject,
		 int type __attribute__((unused)), int depth, int pretty) {

  while (bobject) {

    switch (BOBJECT_TYPE(bobject)) {

    case AMBENCODE_STRING:
      printf("%d:%.*s", BOBJECT_STRING_LEN(bobject), BOBJECT_STRING_LEN(bobject), BOBJECT_STRING_PTR(bhandle, bobject));
      break;
    case AMBENCODE_NUMBER:
      printf("i%.*se", BOBJECT_STRING_LEN(bobject), BOBJECT_STRING_PTR(bhandle, bobject));
      break;
    case AMBENCODE_DICTIONARY:
      printf("d");
      dump(bhandle, DICTIONARY_FIRST_KEY(bhandle, bobject), AMBENCODE_DICTIONARY, depth+1, pretty);
      printf("e");
      break;
    case AMBENCODE_LIST:
      printf("l");
      dump(bhandle, LIST_FIRST(bhandle, bobject), AMBENCODE_LIST, depth+1, pretty);
      printf("e");
      break;
    }

    if (depth == 0) {    
      bobject = (void *)0;
    } else {
      bobject = BOBJECT_NEXT(bhandle,bobject);
    }
  }
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
void ambencode_dump_json(struct bhandle *bhandle, struct bobject *bobject, int pretty) {

  if (!bobject) bobject = BOBJECT_ROOT(bhandle);
  
  dump_json(bhandle, bobject, BOBJECT_TYPE(bobject), 0, pretty);

  printf("\n");
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
void ambencode_dump(struct bhandle *bhandle, struct bobject *bobject, int pretty) {

  if (!bobject) bobject = BOBJECT_ROOT(bhandle);
  
  dump(bhandle, bobject, BOBJECT_TYPE(bobject), 0, pretty);

  printf("\n");
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
