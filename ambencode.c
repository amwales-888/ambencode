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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ambencode.h"

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

int ambencode_document(struct bhandle * const bhandle, char **optr);
int ambencode_dictionary(struct bhandle * const bhandle, char **optr);
int ambencode_list(struct bhandle * const bhandle, char **optr);
int ambencode_value(struct bhandle * const bhandle, char **optr);
int ambencode_string(struct bhandle * const bhandle, char **optr);
int ambencode_number(struct bhandle * const bhandle, char **optr);

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_alloc(struct bhandle * const bhandle, struct bobject *ptr,
		 poff_t count) {

  memset(bhandle, 0, sizeof(struct bhandle));
  
  bhandle->count = count;
  bhandle->root  = AMBENCODE_INVALID;

  if (ptr) {
    bhandle->userbuffer = (unsigned int)1;
    bhandle->bobject    = ptr;
    return 0;
  }

  if (count == 0) goto error;

  if ((bhandle->bobject = (struct bobject *)malloc((size_t)bhandle->count *
						   sizeof(struct bobject)))) {
    return 0;
  }

 error:
  errno = EINVAL;
  return -1;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
void ambencode_free(struct bhandle *bhandle) {

  if (!bhandle->userbuffer) {
    free(bhandle->bobject);
  }
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_decode(struct bhandle * const bhandle, char *buf, bsize_t len) {

  struct bobject *object;
  char *ptr = buf;

  bhandle->buf  = buf;
  bhandle->len  = len;
  bhandle->eptr = &buf[len];
  
  bhandle->max_depth = AMBENCODE_MAXDEPTH;
  bhandle->depth     = 0;

  bhandle->useljmp = 1;
  if (setjmp(bhandle->setjmp_ctx) == 1) {

    /* We returned from calling ambencode_document() with an 
     * allocation failure.
     */
    errno = ENOMEM;
    return -1;
  }
  
  if (!ambencode_document(bhandle, &ptr)) {

    errno = EINVAL;
    return -1;
  }

  /* Our root object can be almost anything.
   */
  object = BOBJECT_LAST(bhandle);
  bhandle->root = BOBJECT_OFFSET(bhandle, object);
  
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
struct bobject *bobject_allocate(struct bhandle * const bhandle, poff_t count) {

  poff_t used = bhandle->used + count;

  if (used <= bhandle->used) goto error; /* overflow */
  
  if (used < bhandle->count) {

    struct bobject *bobject = &bhandle->bobject[bhandle->used];
    bhandle->used = used;
    
    return bobject;
  }

  if (!bhandle->userbuffer) {
    
    void *ptr;
    poff_t ncount = (bhandle->count * 2) + count;
    
    if (ncount <= bhandle->count) goto error; /* overflow */
        
    ptr = realloc(bhandle->bobject, (ncount * sizeof(struct bobject)));	  
    if (ptr) {
      bhandle->count   = ncount;
      bhandle->bobject = (struct bobject *)ptr;
      return bobject_allocate(bhandle, count);
    }
  }

 error:

  if (bhandle->useljmp) {
    /* Jump right back to ambencode_decode() 
     */   
    longjmp(bhandle->setjmp_ctx, 1);  
  }
    
  return (struct bobject *)0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_document(struct bhandle * const bhandle, char **optr) {

  char *ptr = *optr;
  char * const eptr = bhandle->eptr;

  if (eptr == ptr) goto fail;

  do {

    if (!ambencode_value(bhandle, &ptr)) goto fail;

  } while (eptr != ptr);

  if (eptr == ptr) {
    *optr = ptr;
    return 1;
  }
  
 fail:
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_dictionary(struct bhandle * const bhandle, char **optr) {

  char *ptr = *optr;
  char * const eptr = bhandle->eptr;
  struct bobject *object;

  poff_t first  = AMBENCODE_INVALID;
  bsize_t count = 0;

  bhandle->depth++;
  
  if (eptr == ptr) goto fail;
  if ((*ptr == 'd') &&
      (bhandle->depth < bhandle->max_depth)) {

    struct bobject *string;
    struct bobject *value;
    struct bobject *bobject;

    poff_t last = AMBENCODE_INVALID;

    ptr++;

  nextobject:

    if (eptr == ptr) goto fail;  
    if (*ptr == 'e') {
      ptr++;
      goto success;
    } 

    if (!ambencode_string(bhandle, &ptr)) goto fail;

    /* Add string to list */
    count++;
    
    string = BOBJECT_LAST(bhandle);
    if (count == 1) {
      first = BOBJECT_OFFSET(bhandle, string);
      last  = first;
    } else {
      bobject = BOBJECT_AT(bhandle, last);
      bobject->next = BOBJECT_OFFSET(bhandle, string);
      last = bobject->next;
    }
    /* String added */
        
    if (eptr == ptr) goto fail;
    
    if (!ambencode_value(bhandle, &ptr)) goto fail;

    /* Add value to list */
    count++;
    
    value = BOBJECT_LAST(bhandle);
    bobject = BOBJECT_AT(bhandle, last);
    bobject->next = BOBJECT_OFFSET(bhandle, value);
    last = bobject->next;
    /* Value added */
    
    goto nextobject;
  }

  goto fail;

 success:
  
  object = bobject_allocate(bhandle, 1);
 
  object->blen           = count | (AMBENCODE_DICTIONARY << AMBENCODE_LENBITS);
  object->next           = AMBENCODE_INVALID;
  object->u.object.child = first;

  bhandle->depth--;
  *optr = ptr;
  return 1;  

 fail:
  bhandle->depth--;
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_list(struct bhandle * const bhandle, char **optr) {

  char *ptr = *optr;
  char * const eptr = bhandle->eptr;
  struct bobject *array;

  poff_t first  = AMBENCODE_INVALID;
  bsize_t count = 0;

  bhandle->depth++;

  if (eptr == ptr) goto fail;  
  if ((*ptr == 'l') &&
      (bhandle->depth < bhandle->max_depth)) {

    struct bobject *value;
    struct bobject *bobject;
    poff_t last = AMBENCODE_INVALID;
  
    ptr++;

  nextvalue:

    if (eptr == ptr) goto fail;  
    if (*ptr == 'e') {
      ptr++;
      goto success;
    } 
    
    if (eptr == ptr) goto fail;
    
    if (!ambencode_value(bhandle, &ptr)) {
      if (eptr == ptr) goto fail;  
      if (*ptr == 'e') {
	ptr++;
	goto success;
      }
      goto fail;
    }

    /* Add value to list */
    count++;
    
    value = BOBJECT_LAST(bhandle);
    if (count == 1) {
      first = BOBJECT_OFFSET(bhandle, value);
      last  = first;
    } else {
      bobject = BOBJECT_AT(bhandle, last);
      bobject->next = BOBJECT_OFFSET(bhandle, value);
      last = bobject->next;
    }
    /* Value added */
    
    goto nextvalue;
  }

  goto fail;

 success:
 
  array = bobject_allocate(bhandle, 1);
  
  array->blen           = count | (AMBENCODE_LIST << AMBENCODE_LENBITS);
  array->next           = AMBENCODE_INVALID;
  array->u.object.child = first;
  
  bhandle->depth--;

  *optr = ptr;
  return 1;  

 fail:
  bhandle->depth--;
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_value(struct bhandle * const bhandle, char **optr) {
  
  char *ptr = *optr;

  if ((unsigned char)(*ptr - '0') < 10) {
    if (!ambencode_string(bhandle, &ptr)) goto fail;
  } else if (*ptr == 'i') {
    if (!ambencode_number(bhandle, &ptr)) goto fail;
  } else if (*ptr == 'd') {
    if (!ambencode_dictionary(bhandle, &ptr)) goto fail;
  } else if (*ptr == 'l') {
    if (!ambencode_list(bhandle, &ptr)) goto fail;
  } else {
    goto fail;
  }
  
  *optr = ptr;
  return 1;

 fail:
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_string(struct bhandle * const bhandle, char **optr) {

  char *ptr = *optr;
  char * const eptr = bhandle->eptr;
  struct bobject *bobject;
  bsize_t len;
  char *str;
  size_t value = 0;
  
  if (eptr == ptr) goto fail;
  if (*ptr == '0') {
    ptr++;

    if (eptr == ptr) goto fail;
    if (*ptr == ':') {
      ptr++;
      str = ptr;
      goto success;
    }

  } else if ((unsigned char)(*ptr - '1') < 9) {
    value *= 10;
    value += (unsigned char)(*ptr - '0');
    ptr++;
    
  nextdigit:
    
    if (eptr == ptr) goto fail;  
    if ((unsigned char)(*ptr - '0') < 10) {
      value *= 10;
      value += (unsigned char)(*ptr - '0');
      ptr++;
      
      goto nextdigit;
    }

    if (eptr == ptr) goto fail;  
    if (*ptr == ':') {
      ptr++;
      str = ptr;

      ptr += value;
      if (ptr > eptr) goto fail;

      goto success;
    }
  }

  goto fail;

 success:

  len = value;
  if (len > AMBENCODE_MAXSTR) goto fail; 
  
  bobject = bobject_allocate(bhandle, 1);

  bobject->blen            = len | (AMBENCODE_STRING << AMBENCODE_LENBITS);
  bobject->next            = AMBENCODE_INVALID;
  bobject->u.string.offset = (str) - bhandle->buf;

  *optr = ptr;
  return 1;

 fail:
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int ambencode_number(struct bhandle * const bhandle, char **optr) {

  char *ptr = *optr;
  char * const eptr = bhandle->eptr;
  struct bobject *bobject;
  bsize_t len;
  char *str = (char *)0;

  if (eptr == ptr) goto fail;
  if (*ptr == 'i') {
    ptr++;

    if (eptr == ptr) goto fail;  
    if (*ptr == '-') {
      str = ptr;
      ptr++;
    }

    if (eptr == ptr) goto fail;  
    if (*ptr == '0') {
      if (!str) str = ptr;
      ptr++;

      if (eptr == ptr) goto fail;  
      if (*ptr == 'e') {
	ptr++;
	goto success;
      }

    } else if ((unsigned char)(*ptr - '1') < 9) {
      if (!str) str = ptr;
      ptr++;
    
    nextdigit:
    
      if (eptr == ptr) goto fail;  
      if ((unsigned char)(*ptr - '0') < 10) {
	ptr++;
      
	goto nextdigit;
      }

      if (eptr == ptr) goto fail;  
      if (*ptr == 'e') {
	ptr++;
	goto success;
      }
    }
  }

  goto fail;

 success:

  len = (ptr-1) - ((*optr)+1); 
  if (len > 19) goto fail;
  
  bobject = bobject_allocate(bhandle, 1);

  bobject->blen            = len | (AMBENCODE_NUMBER << AMBENCODE_LENBITS);
  bobject->next            = AMBENCODE_INVALID;
  bobject->u.string.offset = (str) - bhandle->buf;

  *optr = ptr;
  return 1;  

 fail:
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
