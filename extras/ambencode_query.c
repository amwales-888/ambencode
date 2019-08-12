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

#include "ambencode.h"
#include "extras/ambencode_query.h"
#include "extras/ambencode_util.h"

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

static char *query_index(char *ptr);
static char *query_identifier(char *ptr);

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static char *query_index(char *ptr) {

  char *optr = ptr;
  
  if (*ptr == '\0') goto fail;
  if (*ptr == '[') {
    ptr++;

    if (*ptr == '\0') goto fail;
    if (*ptr == '0') {
      ptr++;
    } else if ((*ptr >= '1') && (*ptr <= '9')) {
      ptr++;

    nextdigit:
      if ((*ptr >= '0') && (*ptr <= '9')) {
	ptr++;
	goto nextdigit;
      }

    } else {
      goto fail;
    }

    if (*ptr == '\0') goto fail;
    if (*ptr == ']') {
      ptr++;
      goto success;
    }    
  }
  goto fail;

 success:
  return ptr;
 fail:
  return optr;  
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static char *query_identifier(char *ptr) {

  while ((*ptr != '\0') && (*ptr != '[') &&
	 (*ptr != ']') && (*ptr != '.')) {
    
    ptr++;
  }
      
  return ptr;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_query(struct bhandle *bhandle,
			     struct bobject *bobject, char *ptr) {  

  if (*ptr == '\0') goto fail;
  
  for (;;) {
  
    char *nptr;

    nptr = query_index(ptr);
    if (nptr == ptr) {
      nptr = query_identifier(ptr);
      if (nptr == ptr) {
	goto fail;
      } else {

	if (BOBJECT_TYPE(bobject) != AMBENCODE_DICTIONARY) goto fail;       	
	bobject = ambencode_object_find(bhandle, bobject, ptr, (nptr - ptr));
	if (!bobject) goto fail;    

	ptr = nptr;
	if (*ptr == '\0') goto success;
      }
    } else {
      int index = 0;
      
      if (BOBJECT_TYPE(bobject) != AMBENCODE_LIST) goto fail;       	

      ptr++; /* '[' */     
      while (ptr != (nptr-1)) {
	index *= 10;
	index += *ptr - '0';
	ptr++;
      }
      
      ptr++; /* ']' */     

      bobject = ambencode_array_index(bhandle, bobject, index);
      if (!bobject) goto fail;    

      if (*ptr == '\0') goto success;

      if ((*ptr != '.') &&
	  (*ptr != '['))
	goto fail;

      if (*ptr == '.') ptr++;      
    }
    
  }

 success:
  return bobject;
 fail:
  return (struct bobject *)0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
