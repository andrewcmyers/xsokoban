#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "externs.h"
#include "globals.h"

/* Read in the current screen level, updating "ppos", "map", "savepack",
   and "packets" appropriately. Return 0 if success, else an error code. */
short ReadScreen(void)
{
  FILE *screen;
  char *fnam;
  int y, x;
  short j, c, ret = 0;

  for (y = 0; y < MAXROW; y++)
    for (x = 0; x < MAXCOL; x++)
      map[y][x] = ground;

  fnam = malloc(strlen(SCREENPATH) + 12);
  sprintf(fnam, "%s/screen.%d", SCREENPATH, level);

  if ((screen = fopen(fnam, "r")) == NULL)
    ret = E_FOPENSCREEN;
  else {
    packets = savepack = rows = j = cols = 0;
    ppos.x = -1;
    ppos.y = -1;

    while ((ret == 0) && ((c = getc(screen)) != EOF)) {
      if (c == '\n') {
	map[rows++][j] = '\0';
	if (rows > MAXROW)
	  ret = E_TOMUCHROWS;
	else {
	  if (j > cols)
	    cols = j;
	  j = 0;
	}
      } else if ((c == player) || (c == playerstore)) {
	if (ppos.x != -1)
	  ret = E_PLAYPOS1;
	else {
	  ppos.x = rows;
	  ppos.y = j;
	  map[rows][j++] = c;
	  if (j > MAXCOL)
	    ret = E_TOMUCHCOLS;
	}
      } else if ((c == save) || (c == packet) ||
		 (c == wall) || (c == store) ||
		 (c == ground)) {
	if (c == save) {
	  savepack++;
	  packets++;
	}
	if (c == packet)
	  packets++;
	map[rows][j++] = c;
	if (j > MAXCOL)
	  ret = E_TOMUCHCOLS;
      } else
	ret = E_ILLCHAR;
    }
    fclose(screen);
    if ((ret == 0) && (ppos.x == -1))
      ret = E_PLAYPOS2;
  }
  free(fnam);
  return (ret);
}
