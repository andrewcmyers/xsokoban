#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "externs.h"
#include "globals.h"

extern FILE *fopen();

extern char *username;
extern short scorelevel, scoremoves, scorepushes;

static short scoreentries;
static struct {
  char user[MAXUSERNAME];
  unsigned short lv, pad1, mv, pad2, ps, pad3;
} scoretable[MAXSCOREENTRIES];

static FILE *scorefile;
static long sfdbn;

short LockScore(void)
{
     int i, fd;

     for (i = 0; i < 10; i++) {
	  fd = creat(LOCKFILE, 0666);
	  if (fd < 0)
	       sleep(1);
	  else
	       break;
     }

     if (fd < 0) {
	  /* assume that the last process to muck with the score file */
	  /* is dead						      */
	  unlink(LOCKFILE);
	  fd = creat(LOCKFILE, 0666);
     }

     if (fd < 0)
	  return E_WRITESCORE;
     else {
	  close(fd);
	  return 0;
     }
}

void UnlockScore(void)
{
     unlink(LOCKFILE);
}
     
/* print out the score list */
short OutputScore(void)
{
  short ret;

  if (ret = LockScore())
       return ret;

  if ((ret = ReadScore()) == 0)
    ShowScore();
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
}

/* create a new score file */
short MakeNewScore(void)
{
  short ret = 0;

  if (ret = LockScore())
       return ret;
  
  scoreentries = 0;

  if ((scorefile = fopen(SCOREFILE, "w")) == NULL)
    ret = E_FOPENSCORE;
  else {
    sfdbn = fileno(scorefile);
    if (write(sfdbn, &scoreentries, 2) != 2)
      ret = E_WRITESCORE;
    fclose(scorefile);
  }
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
}

/* get the players current level based on the level they last scored on */
short GetUserLevel(short *lv)
{
  short ret = 0, pos;

  if (ret = LockScore())
       return ret;

  if ((scorefile = fopen(SCOREFILE, "r")) == NULL)
    ret = E_FOPENSCORE;
  else {
    if ((ret = ReadScore()) == 0)
      *lv = ((pos = FindUser()) > -1) ? scoretable[pos].lv + 1 : 1;
  }
  UnlockScore();
  return (ret);
}

/* Add a new score to the score file */
short Score(void)
{
  short ret;

  if (ret = LockScore())
       return ret;
  if ((ret = ReadScore()) == 0)
    if ((ret = MakeScore()) == 0)
      if ((ret = WriteScore()) == 0)
	ShowScore();
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
}

/* read in an existing score file.  Uses the ntoh() and hton() functions
 * so that the score files transfer across systems.
 */
short ReadScore(void)
{
  short ret = 0;
  long tmp;

  if ((scorefile = fopen(SCOREFILE, "r")) == NULL)
    ret = E_FOPENSCORE;
  else {
    sfdbn = fileno(scorefile);
    if (read(sfdbn, &scoreentries, 2) != 2)
      ret = E_READSCORE;
    else {
      scoreentries = ntohs(scoreentries);
      tmp = scoreentries * sizeof(scoretable[0]);
      if (read(sfdbn, &(scoretable[0]), tmp) != tmp)
	ret = E_READSCORE;

      /* swap up for little-endian machines */
      for (tmp = 0; tmp < scoreentries; tmp++) {
	scoretable[tmp].lv = ntohs(scoretable[tmp].lv);
	scoretable[tmp].mv = ntohs(scoretable[tmp].mv);
	scoretable[tmp].ps = ntohs(scoretable[tmp].ps);
      }
    }
    fclose(scorefile);
  }
  return ret;
}

/* makes a new user score.  uses the ntoh and hton functions to make sure
 * the score file is transferable across systems
 */
short MakeScore(void)
{
  short ret = 0, pos, i;
  Boolean insert, build = _true_;

  if ((pos = FindUser()) > -1) {  /* user already in score file */
    insert = ((scorelevel > scoretable[pos].lv) ||
	      ((scorelevel == scoretable[pos].lv) &&
	       (scoremoves < scoretable[pos].mv)) ||
	      ((scorelevel == scoretable[pos].lv) &&
	       (scoremoves == scoretable[pos].mv) &&
	       (scorepushes < scoretable[pos].ps)));
    if (insert) {		/* delete existing entry */
      for (i = pos; i < scoreentries - 1; i++)
	CopyEntry(i, i + 1);
      scoreentries--;
    } else
      build = _false_;
  } else if (scoreentries == MAXSCOREENTRIES)
    ret = E_TOMUCHSE;
  if ((ret == 0) && build) {
    pos = FindPos();		/* find the new score position */
    if (pos > -1) {		/* score table not empty */
      for (i = scoreentries; i > pos; i--)
	CopyEntry(i, i - 1);
    } else
      pos = scoreentries;

    strcpy(scoretable[pos].user, username);
    scoretable[pos].lv = scorelevel;
    scoretable[pos].mv = scoremoves;
    scoretable[pos].ps = scorepushes;
    scoreentries++;
  }
  return ret;
}

/* searches the score table to find a specific player */
short FindUser(void)
{
  short i;
  Boolean found = _false_;

  for (i = 0; (i < scoreentries) && (!found); i++)
    found = (strcmp(scoretable[i].user, username) == 0);
  return ((found) ? i - 1 : -1);
}

/* finds the position for a new score in the score table */ 
short FindPos(void)
{
  short i;
  Boolean found = _false_;

  for (i = 0; (i < scoreentries) && (!found); i++)
    found = ((scorelevel > scoretable[i].lv) ||
	     ((scorelevel == scoretable[i].lv) &&
	      (scoremoves < scoretable[i].mv)) ||
	     ((scorelevel == scoretable[i].lv) &&
	      (scoremoves == scoretable[i].mv) &&
	      (scorepushes < scoretable[i].ps)));
  return ((found) ? i - 1 : -1);
}

/* writes out the score table.  It uses ntoh() and hton() functions to make
 * the scorefile transfer across systems.
 */
short WriteScore(void)
{
  short ret = 0;
  long tmp;

  if ((scorefile = fopen(SCOREFILE, "w")) == NULL)
    ret = E_FOPENSCORE;
  else {
    sfdbn = fileno(scorefile);
    scoreentries = htons(scoreentries);
    if (write(sfdbn, &scoreentries, 2) != 2)
      ret = E_WRITESCORE;
    else {
      scoreentries = ntohs(scoreentries);

      /* swap around for little-endian machines */
      for (tmp = 0; tmp < scoreentries; tmp++) {
	scoretable[tmp].lv = htons(scoretable[tmp].lv);
	scoretable[tmp].mv = htons(scoretable[tmp].mv);
	scoretable[tmp].ps = htons(scoretable[tmp].ps);
      }
      tmp = scoreentries * sizeof(scoretable[0]);
      if (write(sfdbn, &(scoretable[0]), tmp) != tmp)
	ret = E_WRITESCORE;

      /* and swap back for the rest of the run ... */
      for (tmp = 0; tmp < scoreentries; tmp++) {
	scoretable[tmp].lv = ntohs(scoretable[tmp].lv);
	scoretable[tmp].mv = ntohs(scoretable[tmp].mv);
	scoretable[tmp].ps = ntohs(scoretable[tmp].ps);
      }
    }
    fclose(scorefile);
  }
  return ret;
}

/* displays the score table to the user */
void ShowScore(void)
{
  register short lastlv = 0, lastmv = 0, lastps = 0, i;

  fprintf(stdout, "Rank        User     Level     Moves    Pushes\n");
  fprintf(stdout, "==============================================\n");
  for (i = 0; i < scoreentries; i++) {
    if ((scoretable[i].lv == lastlv) &&
	(scoretable[i].mv == lastmv) &&
	(scoretable[i].ps == lastps))
      fprintf(stdout, "      ");
    else {
      lastlv = scoretable[i].lv;
      lastmv = scoretable[i].mv;
      lastps = scoretable[i].ps;
      fprintf(stdout, "%4d  ", i + 1);
    }
    fprintf(stdout, "%10s  %8d  %8d  %8d\n", scoretable[i].user,
	    scoretable[i].lv, scoretable[i].mv, scoretable[i].ps);
  }
}

/* duplicates a score entry */
void CopyEntry(short i1, short i2)
{
  strcpy(scoretable[i1].user, scoretable[i2].user);
  scoretable[i1].lv = scoretable[i2].lv;
  scoretable[i1].mv = scoretable[i2].mv;
  scoretable[i1].ps = scoretable[i2].ps;
}
