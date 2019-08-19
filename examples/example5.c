/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>

#include "ambencode.h"
#include "extras/ambencode_dump.h"
#include "extras/ambencode_query.h"
#include "extras/ambencode_mod.h"

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
int main(int argc __attribute__((unused)),
	 char **argv __attribute__((unused))) {

  struct bhandle bhandle;
  char ambencode[] = "d9:publisher3:bob17:publisher-webpage15:www.example.com18:publisher.location4:homee";

  printf("%s\n", ambencode);

  if (ambencode_alloc(&bhandle, (void *)0, 32) == 0) {
    if (ambencode_decode(&bhandle, ambencode, strlen(ambencode)) == 0) {

      struct bobject *bobject= ambencode_query(&bhandle, BOBJECT_ROOT(&bhandle), "publisher-webpage");      
      
      ambencode_update(bobject,
		       ambencode_string_new(&bhandle, "mywebpage", strlen("mywebpage")));

      ambencode_dump(&bhandle, BOBJECT_ROOT(&bhandle), 1, (char *)0, 0);	
      printf("\n");
    }
    ambencode_free(&bhandle);
  }
  
  return 0;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
