#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <assert.h>
#include "externs.h"
#include "globals.h"

extern FILE *fopen();

extern char *username;
extern short scorelevel, scoremoves, scorepushes;

static short scoreentries;
static struct st_entry {
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
	  /* XXX Should really be checking the datestamps on the score*/
	  /* file to make sure some other process hasn't mucked with  */
	  /* in the last 10 seconds! */
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

/* Add a new score to the score file. Show the current scores if "show". */
short Score(Boolean show)
{
  short ret;

  if (ret = LockScore())
       return ret;
  if ((ret = ReadScore()) == 0)
    if ((ret = MakeScore()) == 0)
      if ((ret = WriteScore()) == 0)
	if (show) ShowScore();
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
}

ntohs_entry(struct st_entry *entry)
{
    entry->lv = ntohs(entry->lv);
    entry->mv = ntohs(entry->mv);
    entry->ps = ntohs(entry->ps);
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
      for (tmp = 0; tmp < scoreentries; tmp++) ntohs_entry(&scoretable[tmp]);
    }
    fclose(scorefile);
  }
  return ret;
}

/* Return the solution rank for table index "j". The solution rank for
   an entry is one greater than the number of entries that are better
   than it, unless there is a better or equal solution that is by the
   same person, in which case the solution rank is at least "BADSOLN".
   If two solutions are equal, the one that was arrived at first, and
   thus has a lower table index, is considered to be better.
   One solution is at least as good as another solution if it is at
   least as good in numbers of moves and pushes. Note that
   non-comparable solutions may exist.
*/
#define BADSOLN 100
int SolnRank(int j)
{
    int i, rank = 1;
    short level = scoretable[j].lv;
    for (i = 0; i < j; i++) {
	if (scoretable[i].lv == level) {
	    if (scoretable[i].mv <= scoretable[j].mv &&
		scoretable[i].ps <= scoretable[j].ps)
	    {
		if (0 == strcmp(scoretable[i].user,
				scoretable[j].user))
		    rank = BADSOLN;
		else
		    rank++;
	    }
	}
    }
    return rank;
}

/* Removes all score entries for a user who has multiple entries,
 * that are for a level below the user's top level, and that are not "best
 * solutions" as defined by "SolnRank". Also removes duplicate entries
 * for a level that is equal to the user's top level, but which are not
 * the user's best solution as defined by table position.
 *
 * The current implementation is O(n^2) in the number of actual score entries.
 * A hash table would fix this.
 */
void CleanupScoreTable()
{
    int i,k;
    Boolean deletable[MAXSCOREENTRIES];
    for (i = 0; i < scoreentries; i++) {
	deletable[i] = _false_;
	if (SolnRank(i) > MAXSOLNRANK) {
	    char *user = scoretable[i].user;
	    int j;
	    for (j = 0; j < i; j++) {
		if (0 == strcmp(scoretable[j].user, user))
		  deletable[i] = _true_;
	    }
	}
    }
    k = 0;
    for (i = 0; i < scoreentries; i++) {
	if (i != k) CopyEntry(k, i);
	if (!deletable[i]) k++;
    }
    scoreentries = k;
}

/* Adds a new user score to the score table, if appropriate. Users' top
 * level scores, and the best scores for a particular level (in moves and
 * pushes, separately considered), are always preserved.
 */
short MakeScore(void)
{
  short ret = 0, pos, i;
  Boolean insert, build = _true_;

  pos = FindPos();		/* find the new score position */
  if (pos > -1) {		/* score table not empty */
      for (i = scoreentries; i > pos; i--)
	CopyEntry(i, i - 1);
    } else {
      pos = scoreentries;
    }

  strcpy(scoretable[pos].user, username);
  scoretable[pos].lv = scorelevel;
  scoretable[pos].mv = scoremoves;
  scoretable[pos].ps = scorepushes;
  scoreentries++;

  CleanupScoreTable();
  if (scoreentries == MAXSCOREENTRIES)
    return E_TOMUCHSE;
  else
    return 0;
}


/* searches the score table to find a specific player. */
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
  register i;

  fprintf(stdout, "Rank      User     Level     Moves    Pushes\n");
  fprintf(stdout, "============================================\n");
  for (i = 0; i < scoreentries; i++) {
    int rank = SolnRank(i);
    if (rank <= MAXSOLNRANK) fprintf(stdout, "%4d", SolnRank(i));
    else fprintf(stdout, "    ");
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
