/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>

#include "ambencode.h"
#include "extras/ambencode_query.h"
#include "extras/ambencode_dump.h"

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int main(int argc __attribute__((unused)),
	 char **argv __attribute__((unused))) {

  struct bhandle bhandle;
  char *ambencode = "d4:name3:bobe";
  
  if (ambencode_alloc(&bhandle, (void *)0, 32) == 0) {
    if (ambencode_decode(&bhandle, ambencode, strlen(ambencode)) == 0) {
      struct bobject *bobject;
     
      if ((bobject = ambencode_query(&bhandle, BOBJECT_ROOT(&bhandle), "name"))) {
	ambencode_dump(&bhandle, bobject, 1);
      }
    }
    ambencode_free(&bhandle);
  }
  
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
