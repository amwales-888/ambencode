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
#include <stdarg.h>

#include "ambencode.h"
#include "extras/ambencode_mod.h"

/* -------------------------------------------------------------------- */

extern struct bobject *bobject_allocate(struct bhandle *bhandle, poff_t count);

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

static poff_t ambencode_strdup(struct bhandle *bhandle, char *ptr, bsize_t len);

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static poff_t ambencode_strdup(struct bhandle *bhandle, char *ptr, bsize_t len) {

  char *dptr = (char *)bobject_allocate(bhandle,
					(len + (sizeof(struct bobject)-1)) / sizeof(struct bobject));

  if (!dptr) return (poff_t)-1;
  
  memcpy(dptr, ptr, len);
  return BOBJECT_OFFSET(bhandle, dptr);
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_string_new(struct bhandle *bhandle,
				     char *ptr, bsize_t len) {

  /* TODO - check len overflow */

  poff_t offset = ambencode_strdup(bhandle, ptr, len);
  if (offset != (poff_t)-1) {
  
    struct bobject *bobject = bobject_allocate(bhandle, 1);
    if (!bobject) return (struct bobject *)0;

    bobject->blen            = len | AMBENCODE_STRBUFMASK | (AMBENCODE_STRING << AMBENCODE_LENBITS);
    bobject->next            = AMBENCODE_INVALID;
    bobject->u.string.offset = offset;
    return bobject;
  }
  
  return (struct bobject *)0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_dictionary_add(struct bhandle *bhandle,
					 struct bobject *object,
					 struct bobject *string,
					 struct bobject *value) {

  if (BOBJECT_TYPE(object) == AMBENCODE_DICTIONARY) {
    
    string->next = BOBJECT_OFFSET(bhandle, value);
    
    if (DICTIONARY_COUNT(object) == 0) {
      
      object->u.object.child = BOBJECT_OFFSET(bhandle, string);
      
    } else {
      
      struct bobject *bobject;
      poff_t next = object->u.object.child;
      
      for (;;) {
	
	bobject = BOBJECT_AT(bhandle, next);
	if (bobject->next == AMBENCODE_INVALID) break;
	
	next = bobject->next;
      }
      
      bobject->next = BOBJECT_OFFSET(bhandle, string);
    }
    
    object->blen = (DICTIONARY_COUNT(object) + 2) | (AMBENCODE_DICTIONARY << AMBENCODE_LENBITS);
    return object;
  }
  
  return (struct bobject *)0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_dictionary_new(struct bhandle *bhandle, ...) {

  struct bobject *object;
  va_list ap;

  poff_t last  = AMBENCODE_INVALID;
  poff_t first = AMBENCODE_INVALID;

  bsize_t count = 0;
  
  va_start(ap, bhandle);

  for(;;) {
    
    struct bobject *string;
    struct bobject *value;
    struct bobject *bobject;

    string = va_arg(ap, struct bobject *);
    if (!string) {
      break;
    }

    count++;
    if (last == AMBENCODE_INVALID) {
      first = BOBJECT_OFFSET(bhandle, string);
      last  = first;
    } else {
      bobject = BOBJECT_AT(bhandle, last);
      bobject->next = BOBJECT_OFFSET(bhandle, string);
      last = bobject->next;
    }

    value = va_arg(ap, struct bobject *);

    count++;
    bobject = BOBJECT_AT(bhandle, last);
    bobject->next = BOBJECT_OFFSET(bhandle, value);
    last = bobject->next;
  }

  va_end(ap);

  object = bobject_allocate(bhandle, 1);
  if (!object) return (struct bobject *)0;

  object->blen           = count | (AMBENCODE_DICTIONARY << AMBENCODE_LENBITS);
  object->next           = AMBENCODE_INVALID;
  object->u.object.child = first;
  return object;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_list_new(struct bhandle *bhandle, ...) {

  struct bobject *array;
  va_list ap;

  poff_t last  = AMBENCODE_INVALID;
  poff_t first = AMBENCODE_INVALID;

  bsize_t count = 0;
  
  va_start(ap, bhandle);

  for(;;) {
    
    struct bobject *value = va_arg(ap, struct bobject *);
    if (!value) {
      break;
    }

    count++;
    if (last == AMBENCODE_INVALID) {
      first = BOBJECT_OFFSET(bhandle, value);
      last  = first;
    } else {
      struct bobject *bobject = BOBJECT_AT(bhandle, last);

      bobject->next = BOBJECT_OFFSET(bhandle, value);
      last = bobject->next;
    }
  }
  
  va_end(ap);

  array = bobject_allocate(bhandle, 1);
  if (!array) return (struct bobject *)0;

  array->blen           = count | (AMBENCODE_LIST << AMBENCODE_LENBITS);
  array->next           = AMBENCODE_INVALID;
  array->u.object.child = first;
  return array;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_list_add(struct bhandle *bhandle,
				    struct bobject *array,
				    struct bobject *value) {
  
  if (BOBJECT_TYPE(array) == AMBENCODE_LIST) {
    
    if (LIST_COUNT(array) == 0) {
      
      array->u.object.child = BOBJECT_OFFSET(bhandle, value);
      
    } else {
      
      struct bobject *bobject;
      poff_t next = array->u.object.child;
      
      for (;;) {
	
	bobject = BOBJECT_AT(bhandle, next);
	if (bobject->next == AMBENCODE_INVALID) break;
	
	next = bobject->next;
      }
      
      bobject->next = BOBJECT_OFFSET(bhandle, value);
    }
    
    array->blen = (LIST_COUNT(array) + 1) | (AMBENCODE_LIST << AMBENCODE_LENBITS);
    return array;
  }

  return (struct bobject *)0;
}


/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *ambencode_update(struct bobject *old,
				 struct bobject *new) {
  
  poff_t next = old->next;
  memcpy(old, new, sizeof(struct bobject));
  old->next = next;

  return (struct bobject *)old;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
