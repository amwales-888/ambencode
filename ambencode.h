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

/* This project provides a BENCODE parser that is able to
 * successfully parse correct BENCODE formatted data as described in the 
 * BENCODE grammar available at https://www.json.org/
 *
 * As per RFC 8259 section (9) we set a maximum depth when parsing 
 * elements, this is configurable and a default compile time constant 
 * has been provided.
 * 
 * This library is usable with just 2 files being required ambencode.h and 
 * ambencode.c ALL other files are optional.
 * See the extras directory for additional features, including dumping,
 * pretty printing, object queries and BENCODE DOM creation.
 */

#ifndef _AMBENCODE_H_
#define _AMBENCODE_H_

/* -------------------------------------------------------------------- */

#include <setjmp.h>
#include <sys/types.h>
#include <stdint.h>

/* -------------------------------------------------------------------- */

#define AMBENCODE_MAXDEPTH 64     /* Set the maximum depth we will allow
				   * lists and disctions to descend. Since we
				   * use a recursive descent parser this is 
				   * also affects the maximum stack depth used.
				   * You may lower this number but it will affect 
				   * the maximum nesting of your BENCODE objects */

#define AMBENCODE_12              /* 32bit offsets ( see table** ) */
/* #define AMBENCODE_6 */         /* 16bit offsets ( see table** ) */
/* #define AMBENCODE_3 */         /*  8bit offsets ( see table** ) */

/* #define AMBIGBENCODE */           /* Set string offset to use 'unsigned long',
				   * on 64 bit Linux platforms, this will allow 
				   * us to index strings at offset >4GB The 
				   * downside of this is that every bobject will 
				   * now consume 16bytes instead of 12bytes on a 
				   * 64 bit platform */

/* #define USECOMPUTEDGOTO */     /* Use GCC extension for computed gotos */
/* #define USEBRANCHHINTS */      /* Use hints to aid branch prediction */


/* -------------------------------------------------------------------- *

   Bobject Pool                This is a collection of equal sized structures.
   +----+----+----+----+----+  They are chained via a next (offset).
   |    |    |    |    |    |  The Bobject pool is either unmanaged, that is 
   +---|+-^-|+-^--+----+----+  allocated by the user OR maneged, that is 
       |  | |  |               automaticaly managed by the system. An 
       +--+ +--+               unmanaged pool will return ENOMEM in errno 
                               if it becomes full. A managed pool will be
			       reallocated, if there is no available memory
			       to reallocate ENOMEM will be returned in errno.

   BENCODE Buffer                 This is a collection of bytes that contains
   +---------------+           the BENCODE data that is to be parsed after the 
   |{ "key": 1234 }|           call to ambencode_decode() The BENCODE buffer is never 
   +---------------+           modified but MUST remain accessible to the 
                               system once parsed. This is because the system 
                               generates references into the BENCODE Buffer as 
                               it is parsed. After The call to ambencode_free() 
                               the buffer is no longer required.

  Type           LP64   LLP64( *Windows )
  char              8       8
  short            16      16
  int              32      32
  long             64      32
  long long        64      64
  pointer          64      64

  Linux and MacOS X LP64, Windows LLP64 

  **                        AMBENCODE_3 AMBENCODE_6   AMBENCODE_12     
  MAX Array Entries         63          16383         1073741823
  MAX Object Entries*       31          8191          536870911   *( Key/Value Pairs )
  MAX String Length         63          16383         1073741823
  MAX Number Length         63          16383         1073741823
  MAX Jobect Pool Size      255         65535         4294967295
  Max BENCODE Buffer Length 255         65535         4294967295
  Size of Jobject           3 Bytes     6 Bytes       12 Bytes

  When AMBIGBENCODE is defined the addressable BENCODE Buffer length 
  is increased
                               AMBENCODE_3 AMBENCODE_6   AMBENCODE_12
  Max BENCODE Buffer Length    65535       4294967295    (2^64)-1
  Size of Jobject AMBIGBENCODE 4 Bytes     8 Bytes       16 Bytes

 * -------------------------------------------------------------------- */

#ifdef AMBENCODE_12
typedef uint32_t bsize_t;
typedef uint32_t poff_t;
#define BSIZE_MAX       UINT32_MAX
#define POFF_MAX        UINT32_MAX
#define AMBENCODE_TYPEBITS 2
#define AMBENCODE_LENBITS  30

#ifdef AMBIGBENCODE
typedef uint64_t xbsize_t;     
typedef uint64_t xboff_t;     /* Offset of character into BENCODE buffer */
#define XBOFF_MAX        UINT64_MAX 
#else
typedef uint32_t xbsize_t;     
typedef uint32_t xboff_t;
#define XBOFF_MAX        UINT32_MAX
#endif
#endif

#ifdef AMBENCODE_6
typedef uint16_t bsize_t;
typedef uint16_t poff_t;
#define BSIZE_MAX       UINT16_MAX
#define POFF_MAX        UINT16_MAX
#define AMBENCODE_TYPEBITS 2
#define AMBENCODE_LENBITS  14

#ifdef AMBIGBENCODE
typedef uint32_t xbsize_t;     
typedef uint32_t xboff_t;
#define XBOFF_MAX        UINT32_MAX
#else
typedef uint16_t xbsize_t;     
typedef uint16_t xboff_t;
#define XBOFF_MAX        UINT16_MAX
#endif
#endif

#ifdef AMBENCODE_3
typedef uint8_t bsize_t;
typedef uint8_t poff_t;
#define BSIZE_MAX       UINT8_MAX
#define POFF_MAX        UINT8_MAX

#define AMBENCODE_TYPEBITS 2
#define AMBENCODE_LENBITS  6

#ifdef AMBIGBENCODE
typedef uint16_t xbsize_t;     
typedef uint16_t xboff_t;
#define XBOFF_MAX        UINT16_MAX
#else
typedef uint8_t xbsize_t;     
typedef uint8_t xboff_t;
#define XBOFF_MAX        UINT8_MAX
#endif
#endif

#define AMBENCODE_LENMASK    ((BSIZE_MAX) >> (AMBENCODE_TYPEBITS))
#define AMBENCODE_TYPEMASK   ((BSIZE_MAX) << (AMBENCODE_LENBITS))

/* String/Numbers are found in either the BENCODE buffer (STRBENCODEBUF) 
 * or the Bobject Pool (STRBOBJECTPOOL)
 */
#define AMBENCODE_STRLENMASK ((BSIZE_MAX) >> (AMBENCODE_TYPEBITS+1))
#define AMBENCODE_MAXSTR     AMBENCODE_STRLENMASK
#define STRBENCODEBUF        0
#define STRBOBJECTPOOL    1
#define AMBENCODE_STRBUFMASK (1 << (AMBENCODE_LENBITS-1))

/* -------------------------------------------------------------------- */

struct bobject {

#define AMBENCODE_DICTIONARY 0
#define AMBENCODE_LIST       1 
#define AMBENCODE_STRING     2 
#define AMBENCODE_NUMBER     3 

  bsize_t blen;                   /* type:len packed BENCODE_TYPEBITS 
                                   * and AMBENCODE_LENBITS */
  union {
    struct {
      poff_t  child;              /* Index of first child */
    } object;

    struct {
      xboff_t  offset;             /* First character Offset from start of 
                                   * AMBENCODE buffer */ 
    } string;
  } u;

#define AMBENCODE_INVALID    0    /* Next offset use as value indicating 
                                   * end of list */

  poff_t next;                    /* next offset into bobject pool */

} __attribute__((packed));

struct bhandle {

  char           *buf;            /* Unparsed json data, the BENCODE buffer */
  char           *eptr;           /* Pointer to character after the end of 
                                   * the BENCODE buffer */
  unsigned int   userbuffer:1;    /* Did user supply the buffer? */
  unsigned int   useljmp:1;       /* We want to longjmp on allocation failure */

  xbsize_t       len;             /* Length of json data */  
  jmp_buf        setjmp_ctx;      /* Allows us to return from allocation failure 
				   * from deeply nested calls */
  
  struct bobject *bobject;        /* Preallocated bobject pool */
  poff_t         count;           /* Size of bobject pool */
  poff_t         used;            /* Bobjects in use */
  poff_t         root;            /* Index of our root object */

  int            depth;
  int            max_depth;       /* RFC 8259 section 9 allows us to set a 
                                   * max depth for list and object traversal */
};

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

#define BOBJECT_LAST(bhandle)          (&(bhandle)->bobject[(bhandle)->used-1])
#define BOBJECT_OFFSET(bhandle, o)     ((((char *)(o)) - ((char *)&(bhandle)->bobject[0]))) / sizeof(struct bobject)
#define BOBJECT_AT(bhandle, offset)    (&(bhandle)->bobject[(offset)])

/* -------------------------------------------------------------------- */

#define BOBJECT_ROOT(bhandle)          (BOBJECT_AT((bhandle), (bhandle)->root))
#define BOBJECT_NEXT(bhandle,o)        ((((o)->next) == AMBENCODE_INVALID)?(struct bobject *)0:(BOBJECT_AT((bhandle), ((o)->next))))
#define BOBJECT_TYPE(o)                ((o)->blen >> AMBENCODE_LENBITS)

#define BOBJECT_STRING_LEN(o)          ((o)->blen & AMBENCODE_STRLENMASK)
#define BOBJECT_STRING_PTR(bhandle, o) (((o)->blen & AMBENCODE_STRBUFMASK)?((char *)(&(bhandle)->bobject[(o)->u.string.offset])):(&((bhandle)->buf[(o)->u.string.offset])))

#define LIST_COUNT(o)                 ((o)->blen & AMBENCODE_LENMASK)
#define LIST_FIRST(bhandle, o)        ((((o)->blen & AMBENCODE_LENMASK) == 0)?(struct bobject *)0:(BOBJECT_AT((bhandle),(o)->u.object.child)))
#define LIST_NEXT(bhandle, o)         ((((o)->next) == AMBENCODE_INVALID)?(struct bobject *)0:(BOBJECT_AT((bhandle), ((o)->next))))
#define DICTIONARY_COUNT(o)                ((o)->blen & AMBENCODE_LENMASK)
#define DICTIONARY_FIRST_KEY(bhandle, o)   ((((o)->blen & AMBENCODE_LENMASK) == 0)?(struct bobject *)0:(BOBJECT_AT((bhandle),(o)->u.object.child)))
#define DICTIONARY_NEXT_KEY(bhandle, o)    ((((o)->next) == AMBENCODE_INVALID)?(struct bobject *)0:((BOBJECT_AT((bhandle), ((o)->next))->next == AMBENCODE_INVALID)?(struct bobject *)0:BOBJECT_AT((bhandle),BOBJECT_AT((bhandle), ((o)->next))->next)))
#define DICTIONARY_FIRST_VALUE(bhandle, o) ((((o)->blen & AMBENCODE_LENMASK) == 0)?(struct bobject *)0:BOBJECT_AT((bhandle), BOBJECT_AT((bhandle), (o)->u.object.child)->next))
#define DICTIONARY_NEXT_VALUE(bhandle, o)    ((((o)->next) == AMBENCODE_INVALID)?(struct bobject *)0:((BOBJECT_AT((bhandle), ((o)->next))->next == AMBENCODE_INVALID)?(struct bobject *)0:BOBJECT_AT((bhandle),BOBJECT_AT((bhandle), ((o)->next))->next)))
#define BOBJECT_STRDUP(o)              ((BOBJECT_TYPE((o)) != AMBENCODE_STRING)?((struct bobject *)0):strndup(BOBJECT_STRING_PTR((o)),BOBJECT_STRING_LEN((o))))

#define BOBJECT_P                      6
#define BOBJECT_COUNT_GUESS(size)      ((((size) / BOBJECT_P)<BOBJECT_P)?BOBJECT_P:((size) / BOBJECT_P))

/* -------------------------------------------------------------------- */

#ifndef __cplusplus
#define REGISTER register
#else
#define REGISTER
#endif

#ifdef __cplusplus
extern "C" {  
#endif

/* -------------------------------------------------------------------- */

/* Summary: Create a new ambencode context.
 * bhandle: This is a pointer to an uninitialised bhandle structure.
 *          On success the context will be initialised, a subsequent call
 *          to ambencode_free() must be made to release any resources held.
 * ptr:     This is a pointer to an allocated bobject pool. 
 *          If this is (struct bobject *)0 the bobject pool will be 
 *          allocated by this call.
 * count:   This is the count of struct bobject object in the pool or
 *          if the bobject pool is to be allocated, it's initial count.
 *
 * Return 0 on success and !0 on failure.
 */
int ambencode_alloc(struct bhandle *bhandle, struct bobject *ptr, poff_t count);

/* Summary: Decode a buffer holding BENCODE data using the ambencode context 
 *          allocated by the call to ambencode_alloc()
 * bhandle: This is a pointer to an initialised bhandle structure.
 * buf:     This is a pointer to a buffer holding BENCODE data to be parsed.
 *          The contents of this buffer MUST not be freed or changed while
 *          the ambencode context exists.
 * len:     This is the length of the BENCODE buffer in bytes.
 *
 * Return 0 on success and !0 on failure.
 * The value of errno will be set to EINVAL if an error ocurred parsing
 * the BENCODE buffer. ENOMEM indicates a problem allocating an object from
 * the bobject pool.
 */
int ambencode_decode(struct bhandle *bhandle, char *buf, xbsize_t len);

/* Summary: Release any resources held by an initialised ambencode context.
 * bhandle: This is a pointer to an initialised bhandle structure.
 */
void ambencode_free(struct bhandle *bhandle);

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
#endif
