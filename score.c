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
#include <fcntl.h>
#include <ctype.h>

#include "externs.h"
#include "globals.h"

#define SCORE_VERSION "xs01"

short scoreentries;
struct st_entry scoretable[MAXSCOREENTRIES];

static FILE *scorefile;
static int sfdbn;


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

static time_t lock_time;
/* This timer is used to allow the writer to back out voluntarily if it
   notices that its time has expired. This is not a guarantee that no
   conflicts will occur, since the final rename() in WriteScore could
   take arbitrarily long, running the clock beyond TIMEOUT seconds.
*/

short LockScore(void)
{
#if !WWW
     int i, result;

     for (i = 0; i < TIMEOUT; i++) {
	  result = mkdir(LOCKFILE, 0);
	  lock_time = time(0);
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
	 if (t - s.st_ctime <= TIMEOUT) {
	     fprintf(stderr,
	 "Warning: some other process is mucking with the lock file\n");
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
	 if (result < 0) {
	     fprintf(stderr, "Warning: Couldn't create %s\n", LOCKFILE);
	     return E_WRITESCORE;
	 }
     }
     
     return 0;
#else
     return 0;
#endif
}

void UnlockScore(void)
{
#if !WWW
     if (0 > rmdir(LOCKFILE)) {
	 fprintf(stderr, "Warning: Couldn't remove lock %s\n", LOCKFILE);
     }
#endif
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
#if !WWW
  short ret = 0;

  if ((ret = LockScore()))
       return ret;
  
  scoreentries = 0;

  if ((scorefile = fopen(SCOREFILE, "w")) == NULL)
    ret = E_FOPENSCORE;
  else {
    sfdbn = fileno(scorefile);
    if (write(sfdbn, SCORE_VERSION, 4) != 4)
      ret = E_WRITESCORE;
    else if (write(sfdbn, &scoreentries, 2) != 2)
      ret = E_WRITESCORE;
    fclose(scorefile);
  }
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
#else
    fprintf(stderr, "Cannot make a new score in WWW mode\n");
#endif
}

/* get the players current level based on the level they last scored on */
short GetUserLevel(short *lv)
{
  short ret = 0, pos;

#if !WWW
  if ((ret = LockScore()))
       return ret;

  if ((scorefile = fopen(SCOREFILE, "r")) == NULL)
    ret = E_FOPENSCORE;
  else {
#endif
    if ((ret = ReadScore()) == 0)
      *lv = ((pos = FindUser()) > -1) ? scoretable[pos].lv + 1 : 1;
#if !WWW
  }
  UnlockScore();
#endif
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
short ReadScore_WWW();
short ReadScore(void)
{
#if WWW
    return ReadScore_WWW();
#else
  short ret = 0;
  long tmp;

  sfdbn = open(SCOREFILE, O_RDONLY);
  if (0 > sfdbn)
    ret = E_FOPENSCORE;
  else {
    char magic[5];
    if (read(sfdbn, &magic[0], 4) != 4) ret = E_READSCORE;
    magic[4] = 0;
    if (0 == strcmp(magic, SCORE_VERSION)) {
	/* we have the right version */
    } else {
	fprintf(stderr, "Warning: old-style score file\n");
	lseek(sfdbn, 0, SEEK_SET);
    }
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
    (void)close(sfdbn);
  }
  return ret;
#endif
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
     finish Score(), AND someone else decides to break the lock, AND
     they pick the same temporary name, they may write on top of the
     same file. Then we could scramble the score file by moving it with
     alacrity to SCOREFILE before they finish their update. This is
     quite unlikely, but possible.

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

short WriteScore_WWW();

short WriteScore(void)
{
#if WWW
  return WriteScore_WWW();
#else
  short ret = 0;
  int tmp;
    
  char tempfile[MAXPATHLEN];
  strcpy(tempfile, tempnm);

  (void)mktemp(tempfile);
  scorefile = fopen(tempfile, "w");
  if (!scorefile) return E_FOPENSCORE;
  sfdbn = fileno(scorefile);

  scoreentries = htons(scoreentries);
  if (fwrite(SCORE_VERSION, 4, 1, scorefile) != 1) {
      ret = E_WRITESCORE;
  } else if (fwrite(&scoreentries, 2, 1, scorefile) != 1) {
      ret = E_WRITESCORE;
  } else {
      scoreentries = ntohs(scoreentries);

      /* swap around for little-endian machines */
      for (tmp = 0; tmp < scoreentries; tmp++) {
	scoretable[tmp].lv = htons(scoretable[tmp].lv);
	scoretable[tmp].mv = htons(scoretable[tmp].mv);
	scoretable[tmp].ps = htons(scoretable[tmp].ps);
      }
      tmp = scoreentries;
      while (tmp > 0) {
	int n = fwrite(&(scoretable[scoreentries - tmp]),
			sizeof(struct st_entry),
			tmp,
			scorefile);
	if (n <= 0 && errno) {
	    perror(tempfile);
	    ret = E_WRITESCORE;
	    break;
	}
	tmp -= n;
      }

      /* and swap back for the rest of the run ... */
      for (tmp = 0; tmp < scoreentries; tmp++) {
	scoretable[tmp].lv = ntohs(scoretable[tmp].lv);
	scoretable[tmp].mv = ntohs(scoretable[tmp].mv);
	scoretable[tmp].ps = ntohs(scoretable[tmp].ps);
      }
    }
    if (EOF == fflush(scorefile)) {
	ret = E_WRITESCORE;
	perror(tempfile);
    } else
    if (0 > fsync(sfdbn)) {
	ret = E_WRITESCORE;
	perror(tempfile);
    }
    if (EOF == fclose(scorefile)) {
	ret = E_WRITESCORE;
	perror(tempfile);
    }
    if (ret == 0) {
      time_t t = time(0);
      if (t - lock_time >= TIMEOUT) {
	  fprintf(stderr,
  "Took more than %d seconds trying to write score file; lock expired.\n",
		  TIMEOUT);
	  ret = E_WRITESCORE;
      } else if (0 > rename(tempfile, SCOREFILE)) {
	  ret = E_WRITESCORE;
      }
    }
    if (ret != 0) (void)unlink(tempfile);
    return ret;
#endif
}


/* displays the score table to the user. If level == 0, show all
   levels. */
void ShowScore(int level)
{
  register i;


    
  fprintf(stdout, "Rank                     User   Level   Moves  Pushes\n");
  fprintf(stdout, "=====================================================\n");
  for (i = 0; i < scoreentries; i++) {
    if (level == 0 || scoretable[i].lv == level) {
	int rank = SolnRank(i, 0);
	if (rank <= MAXSOLNRANK) fprintf(stdout, "%4d", rank);
	else fprintf(stdout, "    ");
	fprintf(stdout, "%25s  %4d     %4d     %4d\n", scoretable[i].user,
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

static int blurt(char *buf, int bufptr, int count, char c)
{
    if (count == 1) {
	buf[bufptr++] = c;
    } else if ((count & 1) && (count <= 9)) {
	buf[bufptr++] = '0' + count;
	buf[bufptr++] = c;
    } else {
	while (count >= 2) {
	    int digit = count/2;
	    if (digit > 9) digit = 9;
	    count -= 2 * digit;
	    if (digit>1) buf[bufptr++] = '0' + digit;
	    buf[bufptr++] = toupper(c);
	}
	if (count) buf[bufptr++] = c;
    }
    return bufptr;
}

static int compress_moves(char *buf)
{
    int i;
    int bufptr = 0;
    int count = 0;
    char lastc = 0;
    for (i = 0; i < moves; i++) {
	char c = move_history[i];
	if (c != lastc && count) {
	    bufptr = blurt(buf, bufptr, count, lastc);
	    count = 1;
	} else {
	    count++;
	}
	lastc = c;
    }
    bufptr = blurt(buf, bufptr, count, lastc);
    assert(bufptr <= MOVE_HISTORY_SIZE);
    return bufptr;
}

static char movelist[MOVE_HISTORY_SIZE];
static int movelen;

/*
    Copy the string in "template" to a newly-allocated string, which
    should be freed with "free". Any occurrences of '$M' are subsituted
    with the current compressed move history. Occurrences of '$L' are
    subsituted with the current level. '$U' is substituted with the
    current username. '$R' is substituted with the current WWW URL.
    '$$' is substituted with the plain character '$'.
*/
char *subst_names(char const *template)
{
    char buffer[4096];
    char *buf = &buffer[0];
    while (*template) {
	 if (*template != '$') {
	    *buf++ = *template++;
	 } else {
	    template++;
	    switch(*template++) {
		case 'L':
		    sprintf(buf, "%d", (int)level);
		    buf += strlen(buf);
		    break;
		case 'U':
		    strcpy(buf, username);
		    buf += strlen(username);
		    break;
		case 'M':
		    strcpy(buf, movelist);
		    buf += strlen(movelist);
		    break;
		case 'N':
		    sprintf(buf, "%d", movelen);
		    buf += strlen(buf);
		    break;
#if 0
		case 'R':
		    strcpy(buf, url);
		    buf += strlen(url);
		    break;
#endif
		case '$':
		    *buf++ = '$';
		    break;
	    }
	 }
    }
    *buf = 0;
    return strdup(buffer);
}

static const char *www_score_command = WWWSCORECOMMAND;

short WriteScore_WWW()
{
    int buflen;
    char *cmd;
    char *result;
    buflen = compress_moves(movelist);
    movelen = buflen;
    movelist[movelen] = 0;
#if 0
    fprintf(stderr, "compressed %d moves to %d characters\n",
		    moves, movelen);
#endif
    cmd = subst_names(www_score_command);
    result = qtelnet(WWWHOST, WWWPORT, cmd);
    free(cmd);
    if (result) {
#if 0
	fprintf(stderr, "%s", result);
#endif
	free(result);
    }
    return 0;
}

/* Extract one line from "text".  Return 0 if there is no line to extract. */
char *getline(char *text, char *linebuf)
{
    if (*text == 0) {
	*linebuf = 0;
	return 0;
    }
    while (*text != '\n' && *text) {
	*linebuf++ = *text++;
    }
    *linebuf = 0;
    return (*text) ? text + 1 : text ;
}

/*
    Yes, it actually parses xsokoban's own output format! Pretty
    disgusting!
*/
short ReadScore_WWW()
{
    char *cmd, *result, *c, *text;
    char *ws = " \t\r\n";
    char line[256];
    char ibuf[20];
    int len;
    movelist[0] = 0;
    cmd = subst_names(WWWREADSCORECMD);
    result = qtelnet(WWWHOST, WWWPORT, cmd);
/* Now, skip past all the initial crud */
    text = result;
    if (!text) return E_READSCORE;
    for (;;)  {
	text = getline(text, line);
	if (line[0] == '=') break;
    }
    scoreentries = 0;
    for (;;) {
	char *user;
	int level, moves, pushes;
	text = getline(text, line);
	if (!text) break;
	user = strtok(line + 4, ws);
	if (!user) break;
	level = atoi(strtok(0, ws));
	moves = atoi(strtok(0, ws));
	pushes = atoi(strtok(0, ws));
	if (level == 0 || moves == 0 || pushes == 0) return E_READSCORE;
	strncpy(scoretable[scoreentries].user, user, MAXUSERNAME);
	scoretable[scoreentries].lv = (unsigned short)level;
	scoretable[scoreentries].mv = (unsigned short)moves;
	scoretable[scoreentries].ps = (unsigned short)pushes;
	scoreentries++;
    }

    free(result);
    return 0;
}
