#include "config_local.h"

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>

#include "externs.h"
#include "globals.h"

short scoreentries;
struct st_entry scoretable[MAXSCOREENTRIES];

static FILE *scorefile;
static long sfdbn;

/* Acquire the lock on the score file. This is done by creating a new
   directory. If someone else holds the lock, the directory will exist.
   Since mkdir() should be done synchronously, even over NFS,  it will
   fail if someone else holds the lock.

   TIMEOUT is the number of seconds which you get to hold the lock on
   the score file for. Also, the number of seconds after which we give
   up on trying to acquire the lock nicely and start trying to break
   the lock. In theory, there are still some very unlikely ways to get
   hosed.  See comments in WriteScore about this. Portable locking over
   NFS is hard.  

   After TIMEOUT seconds, we break the lock, assuming that the old
   lock-holder died in process. If the process that had the lock is
   alive but just very slow, it could end up deleting the changes we
   make, or we could delete its changes. However, the score file can't
   get trashed this way, since rename() is used to update the score file.

   See comments in WriteScore about how the score file can get trashed,
   though it's extremely unlikely.
*/
short LockScore(void)
{
     int i, result;

     for (i = 0; i < TIMEOUT; i++) {
	  result = mkdir(LOCKFILE, 0);
	  if (result < 0) {
	       if (errno == EEXIST) sleep(1);
	       else return E_WRITESCORE;
	  } else {
	      break;
	  }
     }

     if (result < 0) {
	 struct stat s;
	 time_t t = time(0);
	 if (0 > stat(LOCKFILE, &s)) {
	     fprintf(stderr, "Warning: Can't mkdir or stat %s\n", LOCKFILE);
	     return E_WRITESCORE; /* disappeared? */
	 }
	 /* Check to make sure that the lock is still the same one we
	    saw in the first place. This code assumes loosely synchronized
	    clocks. To do it right, we'd have to create another file on
	    the server machine, and compare timestamps. Not worth it.
	 */
	 if (t - s.st_ctime < TIMEOUT) {
	     fprintf(stderr,
     "Warning: some other process is mucking with with the lock file\n");
	     return E_WRITESCORE;
	 }
	 /* Assume that the last process to muck with the score file
	    is dead.
	 */
	 fprintf(stderr, "Warning: breaking old lock\n");
	 if (0 > rmdir(LOCKFILE)) {
	     fprintf(stderr, "Warning: Couldn't remove old lock %s\n",
		     LOCKFILE);
	     return E_WRITESCORE;
	 }
	 result = mkdir(LOCKFILE, 0);
     }
     
     if (result < 0) {
	 fprintf(stderr, "Warning: Couldn't create %s\n", LOCKFILE);
	 return E_WRITESCORE;
     } else {
	 return 0;
     }
}

void UnlockScore(void)
{
     if (0 > rmdir(LOCKFILE)) {
	 fprintf(stderr, "Warning: Couldn't remove lock %s\n", LOCKFILE);
     }
}
     
/* print out the score list for level "level". If "level" == 0, show
   scores for all levels. */
short OutputScore(int level)
{
  short ret;

  if ((ret = LockScore()))
       return ret;

  if ((ret = ReadScore()) == 0)
    ShowScore(level);
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
}

/* create a new score file */
short MakeNewScore(void)
{
  short ret = 0;

  if ((ret = LockScore()))
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

  if ((ret = LockScore()))
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

  if ((ret = LockScore()))
       return ret;
  if ((ret = ReadScore()) == 0)
    if ((ret = MakeScore()) == 0)
      if ((ret = WriteScore()) == 0)
	if (show) ShowScore(0);
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
}

void ntohs_entry(struct st_entry *entry)
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

   The array "ignore" indicates that some scoretable entries should
   be ignored for the purpose of computing rank.
*/
#define BADSOLN 100
int SolnRank(int j, Boolean *ignore)
{
    int i, rank = 1;
    unsigned short level = scoretable[j].lv;
    for (i = 0; i < j; i++) {
	if ((!ignore || !ignore[i]) && scoretable[i].lv == level) {
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
    int i;
    Boolean deletable[MAXSCOREENTRIES];
    for (i = 0; i < scoreentries; i++) {
	deletable[i] = _false_;
	if (SolnRank(i, deletable) > MAXSOLNRANK) {
	    char *user = scoretable[i].user;
	    int j;
	    for (j = 0; j < i; j++) {
		if (0 == strcmp(scoretable[j].user, user))
		  deletable[i] = _true_;
	    }
	}
    }
    FlushDeletedScores(deletable);
}

/* Deletes entries from the score table for which the boolean array
   contains true.
*/
void FlushDeletedScores(Boolean delete[])
{
    int i, k = 0;
    for (i = 0; i < scoreentries; i++) {
	if (i != k) CopyEntry(k, i);
	if (!delete[i]) k++;
    }
    scoreentries = k;
}

/* Adds a new user score to the score table, if appropriate. Users' top
 * level scores, and the best scores for a particular level (in moves and
 * pushes, separately considered), are always preserved.
 */
short MakeScore(void)
{
  short pos, i;

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

/*  WriteScore() writes out the score table.  It uses ntoh() and hton()
    functions to make the scorefile compatible across systems. It and
    LockScore() try to avoid trashing the score file, even across NFS.
    However, they are not perfect.

     The vulnerability here is that if we take more than 10 seconds to
     finish this routine, AND someone else decides to break the lock,
     AND they pick the same temporary name, they may write on top of
     the same file. Then we could scramble the score file by suddenly
     moving it with alacrity to SCOREFILE before they finish their
     update. This is quite unlikely, but possible.

     We could limit the damage by writing just the one score we're
     adding to a temporary file *when we can't acquire the lock*. Then,
     the next time someone comes by and gets the lock, they integrate
     all the temporary files. Since the score change would be smaller
     than one block, duplicate temporary file names means only that a
     score change can be lost. This approach would not require a TIMEOUT.

     The problem with that scheme is that if someone dies holding the
     lock, the temporary files just pile up without getting applied.
     Also, user intervention is required to blow away the lock; and
     blowing away the lock can get us in the same trouble that happens
     here.
*/

char const *tempnm = SCOREFILE "XXXXXX";

short WriteScore(void)
{
  short ret = 0;
  long tmp;

    
  char tempfile[MAXPATHLEN];
  strcpy(tempfile, tempnm);

  (void)mktemp(tempfile);

  if ((scorefile = fopen(tempfile, "w")) == NULL)
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
    if (EOF == fclose(scorefile)) {
	ret = E_WRITESCORE;
	perror(SCOREFILE);
    }
  }
  if (ret == 0) {
      if (0 > rename(tempfile, SCOREFILE)) {
	  return E_WRITESCORE;
      }
  } else {
      (void)unlink(tempfile);
  }
  return ret;
}


/* displays the score table to the user. If level == 0, show all
   levels. */
void ShowScore(int level)
{
  register i;

  fprintf(stdout, "Rank      User     Level     Moves    Pushes\n");
  fprintf(stdout, "============================================\n");
  for (i = 0; i < scoreentries; i++) {
    if (level == 0 || scoretable[i].lv == level) {
	int rank = SolnRank(i, 0);
	if (rank <= MAXSOLNRANK) fprintf(stdout, "%4d", rank);
	else fprintf(stdout, "    ");
	fprintf(stdout, "%10s  %8d  %8d  %8d\n", scoretable[i].user,
		scoretable[i].lv, scoretable[i].mv, scoretable[i].ps);
    }
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
