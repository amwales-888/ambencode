/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>

#include "ambencode.h"
#include "extras/ambencode_dump.h"

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int main(int argc __attribute__((unused)),
	 char **argv __attribute__((unused))) {

  int i;
  struct bhandle bhandle;
  char *ambencode[10] = { "l4:spam4:eggse",
			  "le",
			  "d3:cow3:moo4:spam4:eggse",
			  "d4:spaml1:a1:bee",
			  "d9:publisher3:bob17:publisher-webpage15:www.example.com18:publisher.location4:homee",
			  "de",
			  "i3e",
			  "i-3e",
			  "4:spam",
			  "0:" };
  
		      
  for (i=0; i<10; i++) {

    printf("--%s--\n", ambencode[i]);

    if (ambencode_alloc(&bhandle, (void *)0, 32) == 0) {
      if (ambencode_decode(&bhandle, ambencode[i], strlen(ambencode[i])) == 0) {
	ambencode_dump(&bhandle, BOBJECT_ROOT(&bhandle), 1, (char *)0, 0);	
      }
      ambencode_free(&bhandle);
    }

  }
  
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
