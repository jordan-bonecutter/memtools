/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* memtools_test.c * * * * * * * * * * * * * * * * * * * * * * * * * */
/* 6 august 2020 * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* jordan bonecutter * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "memtools.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

int main(){
  int* data1;
  char* data2;
  int* data3;
  int* data4, *iter;
  char* data5, *data6;;

  data1 = malloc((sizeof *data1)*1000);
  data1[0] = 1;
  memcomment(data1, "this is a pointer that points to 1000 integer values. wow, very cool!");
  memcomment(data1, "another comment guys!");
  memviolated(data1, "This memory has not been violated");

  data2 = malloc((sizeof *data2)*100);
  memcomment(data2, "I allocated this data at %d", (int)clock());
  memcomment_copy(data2, data1);
  strncpy(data2, "hello world!", 100);

  memtest(NULL, "This should fail, I'm testing NULL");

  memtest(data2 + 3, "This shouldn't fail, I'm testing allocated memory");

  data3 = malloc((sizeof *data3)*10);
  data3[10] = 0;
  memcomment(data3, "Purposefully violating memory");
  memviolated(data3, "I purposely violated this memory!");

  data2 = realloc(data2, ((sizeof *data2)*50));

  data4 = calloc(10, sizeof(int));
  for(iter = data4; iter != data4 + 10; ++iter){
    assert(!*iter);
  }

  data5 = strdup(data2);
  data6 = strndup(data2, 3);
  assert(!strcmp(data2, data5));
  assert(strlen(data6) == 3);

  memprint();
  free(data1);
  free(data2);
  free(data3);
  free(data4);
  free(data5);
  free(data6);
  memprint();

  return 0;
}

