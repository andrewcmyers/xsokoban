#include "config_local.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "externs.h"
#include "globals.h"

static long savedbn;
static char *sfname;
static FILE *savefile;
static struct stat sfstat;

/* save out a game in a standard format.  Uses the ntoh functions and hton
 * functions so that the same save file will work on an MSB or LSB system
 * other than that, it is a standard sokoban score file.
 */
short SaveGame(void)
{
  short ret = 0;

  signal(SIGINT, SIG_IGN);
  sfname = malloc(strlen(SAVEPATH) + strlen(username) + 6);
  sprintf(sfname, "%s/%s.sav", SAVEPATH, username);

  packets = htons(packets);
  pushes = htons(pushes);
  moves = htons(moves);
  level = htons(level);
  cols = htons(cols);
  savepack = htons(savepack);
  rows = htons(rows);
  ppos.x = htons(ppos.x);
  ppos.y = htons(ppos.y);

  if ((savefile = fopen(sfname, "w")) == NULL)
    ret = E_FOPENSAVE;
  else {
    savedbn = fileno(savefile);
    if (write(savedbn, &(map[0][0]), MAXROW * MAXCOL) != MAXROW * MAXCOL)
      ret = E_WRITESAVE;
    else if (write(savedbn, &ppos, sizeof(POS)) != sizeof(POS))
      ret = E_WRITESAVE;
    else if (write(savedbn, &level, 2) != 2)
      ret = E_WRITESAVE;
    else if (write(savedbn, &moves, 2) != 2)
      ret = E_WRITESAVE;
    else if (write(savedbn, &pushes, 2) != 2)
      ret = E_WRITESAVE;
    else if (write(savedbn, &packets, 2) != 2)
      ret = E_WRITESAVE;
    else if (write(savedbn, &savepack, 2) != 2)
      ret = E_WRITESAVE;
    else if (write(savedbn, &rows, 2) != 2)
      ret = E_WRITESAVE;
    else if (write(savedbn, &cols, 2) != 2)
      ret = E_WRITESAVE;
    else
      fclose(savefile);
    if (stat(sfname, &sfstat) != 0)
      ret = E_STATSAVE;
    else if ((savefile = fopen(sfname, "a")) == NULL)
      ret = E_FOPENSAVE;
    else if (write(savedbn, &sfstat, sizeof(sfstat)) != sizeof(sfstat))
      ret = E_WRITESAVE;
    fclose(savefile);
  }

  ppos.x = ntohs(ppos.x);
  ppos.y = ntohs(ppos.y);
  pushes = ntohs(pushes);
  moves = ntohs(moves);
  level = ntohs(level);
  packets = ntohs(packets);
  cols = ntohs(cols);
  rows = ntohs(rows);
  savepack = ntohs(savepack);

  if ((ret == E_WRITESAVE) || (ret == E_STATSAVE))
    unlink(sfname);
  signal(SIGINT, SIG_DFL);

  free(sfname);
  return ret;
}

/* loads in a previously saved game */
short RestoreGame(void)
{
  short ret = 0;
  struct stat oldsfstat;

  signal(SIGINT, SIG_IGN);
  sfname = malloc(strlen(SAVEPATH) + strlen(username) + 6);
  sprintf(sfname, "%s/%s.sav", SAVEPATH, username);

  if (stat(sfname, &oldsfstat) < -1)
    ret = E_NOSAVEFILE;
  else {
    if ((savefile = fopen(sfname, "r")) == NULL)
      ret = E_FOPENSAVE;
    else {
      savedbn = fileno(savefile);
      if (read(savedbn, &(map[0][0]), MAXROW * MAXCOL) != MAXROW * MAXCOL)
	ret = E_READSAVE;
      else if (read(savedbn, &ppos, sizeof(POS)) != sizeof(POS))
	ret = E_READSAVE;
      else if (read(savedbn, &level, 2) != 2)
	ret = E_READSAVE;
      else if (read(savedbn, &moves, 2) != 2)
	ret = E_READSAVE;
      else if (read(savedbn, &pushes, 2) != 2)
	ret = E_READSAVE;
      else if (read(savedbn, &packets, 2) != 2)
	ret = E_READSAVE;
      else if (read(savedbn, &savepack, 2) != 2)
	ret = E_READSAVE;
      else if (read(savedbn, &rows, 2) != 2)
	ret = E_READSAVE;
      else if (read(savedbn, &cols, 2) != 2)
	ret = E_READSAVE;
      else if (read(savedbn, &sfstat, sizeof(sfstat)) != sizeof(sfstat))
	ret = E_READSAVE;
    }

    ppos.x = ntohs(ppos.x);
    ppos.y = ntohs(ppos.y);
    level = ntohs(level);
    moves = ntohs(moves);
    pushes = ntohs(pushes);
    packets = ntohs(packets);
    savepack = ntohs(savepack);
    rows = ntohs(rows);
    cols = ntohs(cols);

    unlink(sfname);
  }
  signal(SIGINT, SIG_DFL);
  free(sfname);
  return ret;
}
