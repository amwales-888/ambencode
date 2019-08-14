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

static void ambencode_document(struct bhandle * const bhandle, char **optr);
static void ambencode_dictionary(struct bhandle * const bhandle, char **optr);
static void ambencode_list(struct bhandle * const bhandle, char **optr);
static void ambencode_value(struct bhandle * const bhandle, char **optr);
static void ambencode_string(struct bhandle * const bhandle, char **optr);
static void ambencode_number(struct bhandle * const bhandle, char **optr);

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

  bhandle->buf       = buf;
  bhandle->len       = len;
  bhandle->eptr      = &buf[len];
  bhandle->max_depth = AMBENCODE_MAXDEPTH;
  bhandle->depth     = 0;
  bhandle->useljmp   = 1;

  switch (setjmp(bhandle->setjmp_ctx)) {

  case 1:
    /* We returned from calling ambencode_document() with an 
     * allocation failure.
     */
    errno = ENOMEM;
    return -1;

  case 2:

    /* We returned from calling ambencode_document() with an 
     * parser failure.
     */
    errno = EINVAL;
    return -1;
  }
  
  ambencode_document(bhandle, &ptr);

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
static void ambencode_document(struct bhandle * const bhandle, char **optr) {

  char *ptr = *optr;
  char * const eptr = bhandle->eptr;

  if (eptr == ptr) goto fail;

  do {

    ambencode_value(bhandle, &ptr);

  } while (eptr != ptr);

  if (eptr == ptr) {
    *optr = ptr;
    return;
  }
  
 fail:
  longjmp(bhandle->setjmp_ctx, 2); /* jump back to ambencode_decode() with EINVAL */
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void ambencode_dictionary(struct bhandle * const bhandle, char **optr) {

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

    ambencode_string(bhandle, &ptr);

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
    
    ambencode_value(bhandle, &ptr);

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
  return;  

 fail:
  longjmp(bhandle->setjmp_ctx, 2); /* jump back to ambencode_decode() with EINVAL */
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void ambencode_list(struct bhandle * const bhandle, char **optr) {

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
    
    ambencode_value(bhandle, &ptr);

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
  return;  

 fail:
  longjmp(bhandle->setjmp_ctx, 2); /* jump back to ambencode_decode() with EINVAL */
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void ambencode_value(struct bhandle * const bhandle, char **optr) {
  
  char *ptr = *optr;

  if ((unsigned char)(*ptr - '0') < 10) {
    ambencode_string(bhandle, &ptr);
  } else if (*ptr == 'i') {
    ambencode_number(bhandle, &ptr);
  } else if (*ptr == 'd') {
    ambencode_dictionary(bhandle, &ptr);
  } else if (*ptr == 'l') {
    ambencode_list(bhandle, &ptr);
  } else {
    goto fail;
  }
  
  *optr = ptr;
  return;

 fail:
  longjmp(bhandle->setjmp_ctx, 2); /* jump back to ambencode_decode() with EINVAL */
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void ambencode_string(struct bhandle * const bhandle, char **optr) {

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
  return;

 fail:
  longjmp(bhandle->setjmp_ctx, 2); /* jump back to ambencode_decode() with EINVAL */
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
static void ambencode_number(struct bhandle * const bhandle, char **optr) {

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
  return;  

 fail:
  longjmp(bhandle->setjmp_ctx, 2); /* jump back to ambencode_decode() with EINVAL */
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
