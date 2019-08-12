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

#include <string.h>

#include "ambencode.h"
#include "extras/ambencode_util.h"

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_array_index(struct bhandle *bhandle,
				   struct bobject *array, unsigned int index) {
  poff_t next; 

  if (index >= LIST_COUNT(array)) return (struct bobject *)0;

  next = array->u.object.child;
  while (index--) {
    struct bobject *bobject = BOBJECT_AT(bhandle, next);
    next = bobject->next;    
  }

  return BOBJECT_AT(bhandle, next);
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_object_find(struct bhandle *bhandle,
				   struct bobject *object,
				   char *key,
				   bsize_t len) {
  poff_t next;

  if (DICTIONARY_COUNT(object) == 0) return (struct bobject *)0;

  next = object->u.object.child;
  do {

    struct bobject *bobject = BOBJECT_AT(bhandle, next);

    if ((BOBJECT_STRING_LEN(bobject) == len) &&
	(memcmp(&bhandle->buf[bobject->u.string.offset],
		key, len) == 0)) {
      return BOBJECT_AT(bhandle, bobject->next);
    }

    bobject = BOBJECT_AT(bhandle, bobject->next);
    next = bobject->next;
        
  } while (next != AMBENCODE_INVALID);
  
  return (struct bobject *)0;
}
