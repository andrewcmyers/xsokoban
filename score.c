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
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef HAVE_NL_LANGINFO
#include <langinfo.h>
#endif

#include "externs.h"
#include "globals.h"
#include "score.h"
#if WWW
#include "www.h"
#endif

#define SCORE_VERSION "xs02"

short scoreentries;
struct st_entry scoretable[MAXSCOREENTRIES];

/* Forward decls */
static short MakeScore();
/* Adds a new user score to the score table, if appropriate. Users' top
 * level scores, and the best scores for a particular level (in moves and
 * pushes, separately considered), are always preserved.
 */

static void FlushDeletedScores(Boolean delete[]);
/* Deletes entries from the score table for which the boolean array
   contains true.
*/

static void CopyEntry(short i1, short i2);
/* Duplicate a score entry: overwrite entry i1 with contents of i2. */

static void DeleteLowRanks();
static void CleanupScoreTable();
/* Removes all score entries for a user who has multiple entries,
 * that are for a level below the user's top level, and that are not "best
 * solutions" as defined by "SolnRank". Also removes duplicate entries
 * for a level that is equal to the user's top level, but which are not
 * the user's best solution as defined by table position.
 *
 * The current implementation is O(n^2) in the number of actual score entries.
 * A hash table would fix this.
 */

static short FindPos();
/* Find the position for a new score in the score table */ 

static short ParseScoreText(char *text, Boolean all_users);
static short ParseScoreLine(int i, char **text /* in out */, Boolean all_users);

#if WWW
static short ParseUserLevel(char *text, short *lv);
static short ParsePartialScore(char *text, int *line1, int *line2);
static short GetUserLevel_WWW(short *lv);
static char *subst_names(char const *template);
/*
    Copy the string in "template" to a newly-allocated string, which
    should be freed with "free". Any occurrences of '$M' are subsituted
    with the current compressed move history. Occurrences of '$L' are
    subsituted with the current level. '$U' is substituted with the
    current username. '$R' is substituted with the current WWW URL.
    '$$' is substituted with the plain character '$'.
*/

static short WriteScore_WWW();
/* Write a solution out to the WWW server */

static short ReadScore_WWW();
/* Read in an existing score file.  Uses the ntoh() and hton() functions
 * so that the score files transfer across systems.
 */

#endif

#if !WWW
static short FindUser();
/* Search the score table to find a specific player, returning the
   index of the highest level that user has played, or -1 if none. */
#endif


static short LockScore();
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

static void ShowScore(int level);
/* displays the score table to the user. If level == 0, show all
   levels. */

static void ShowScoreLine(int i);
/* Print out line "i" of the score file. */

static short ReadScore();
/* Read in an existing score file.  Uses the ntoh() and hton() functions
   so that the score files transfer across systems. Update "scoretable",
   "scoreentries", and "date_stamp" appropriately.
*/

#if !WWW
static short WriteScore();
/* Update the score file to contain a new score. See comments below. */

static FILE *scorefile;
static int sfdbn;

static time_t lock_time;
/* This timer is used to allow the writer to back out voluntarily if it
   notices that its time has expired. This is not a guarantee that no
   conflicts will occur, since the final rename() in WriteScore could
   take arbitrarily long, running the clock beyond TIMEOUT seconds.
*/
#endif

static time_t date_stamp;
/* The most recent date stamp on the score file */

short LockScore()
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

void UnlockScore()
{
     DEBUG_SERVER("about to unlock score file");
#if !WWW
     if (0 > rmdir(LOCKFILE)) {
	 fprintf(stderr, "Warning: Couldn't remove lock %s\n", LOCKFILE);
     }
#endif
}
     
short OutputScore(int lev)
{
  short ret;

  DEBUG_SERVER("entering OutputScore");
  if ((ret = LockScore())) {
      DEBUG_SERVER("couldn't lock score file");
      return ret;
  }

  DEBUG_SERVER("score file locked");
#if WWW
  if (lev != 0) {
      int line1, line2;
      level = lev;
      ret = FetchScoreLevel_WWW(&line1, &line2);
      if (ret == E_OUTOFDATE) ret = 0;
  } else {
      ret = ReadScore_WWW();
  }
  if (ret == 0) ShowScore(lev);
#else
  if ((ret = ReadScore()) == 0) ShowScore(lev);
#endif
  UnlockScore();
  DEBUG_SERVER("score file unlocked");
  return ((ret == 0) ? E_ENDGAME : ret);
}

void DumpLinesWithHeader(int top, int bottom)
{
    int i;
    printf("Entries: %d\n", scoreentries);
    printf("Line1: %d\n", scoreentries - 1 - bottom);
    printf("Line2: %d\n", scoreentries - top);
    printf("Date: %ld\n", (long)date_stamp);
    printf("========================================"
	   "==============================\n");
    for (i = bottom; i >= top; i--) ShowScoreLine(i);
}

short OutputScoreLines(int line1, int line2)
{
    short ret;
    DEBUG_SERVER("entering OutputScoreLines");
    if ((ret = LockScore())) {
	DEBUG_SERVER("couldn't lock score file");
	return ret;
    }
    DEBUG_SERVER("score file locked");
    ret = ReadScore();
    UnlockScore();
    DEBUG_SERVER("score file unlocked");
    if (ret == 0) {
	int i;
	int top, bottom;
	DeleteLowRanks();
	if (line1 > scoreentries) line1 = scoreentries;
	if (line2 > scoreentries) line2 = scoreentries;
	bottom = scoreentries - 1 - line1;
	top = scoreentries - line2;

	for (i = top - 1; i >= 0; i--) {
	    if (scoretable[i].lv != scoretable[top].lv) break;
	}
	top = i + 1;
	for (i = bottom + 1; i < scoreentries; i++) {
	    if (scoretable[i].lv != scoretable[bottom].lv) break;
	}
	bottom = i - 1;
	    
	DumpLinesWithHeader(top, bottom);
    }
    return ((ret == 0) ? E_ENDGAME : ret);
}

short MakeNewScore(char *textfile)
{
#if !WWW
  short ret = 0;

  if ((ret = LockScore()))
       return ret;
  
  if (textfile) {
    char *text, *pos, *end;
    int fd;
    struct stat s;
    if (0 > stat(textfile, &s)) {
	perror(textfile);
	return E_FOPENSCORE;
    }
    if (0 > (fd = open(textfile, O_RDONLY))) {
	perror(textfile);
	return E_FOPENSCORE;
    }
    pos = text = (char *)malloc((size_t)s.st_size);
    end = text + s.st_size;
    while (pos < end) {
	int n = read(fd, pos, end - pos);
	switch(n) {
	    case -1: perror(textfile);
		     return E_FOPENSCORE;
	    case 0:  fprintf(stderr, "Unexpected EOF\n");
		     return E_FOPENSCORE;
	    default: pos += n;
		     break;
		     
	}
    }
    (void)close(fd);
    if ((ret = ParseScoreText(text, _true_))) return ret;
    free(text);
  } else {
      scoreentries = 0;
  }

  if ((ret = WriteScore())) return ret;
  UnlockScore();
  return ((ret == 0) ? E_ENDGAME : ret);
#else
    fprintf(stderr, "Cannot make a new score in WWW mode\n");
    return E_WRITESCORE;
#endif
}

short GetUserLevel(short *lv)
{
  short ret = 0;

#if WWW
  return GetUserLevel_WWW(lv);
#else
  if ((ret = LockScore()))
       return ret;

  if ((scorefile = fopen(SCOREFILE, "r")) == NULL)
    ret = E_FOPENSCORE;
  else {
    short pos;
    if ((ret = ReadScore()) == 0)
      *lv = ((pos = FindUser()) > -1) ? scoretable[pos].lv + 1 : 1;
  }
  UnlockScore();
#endif
  return (ret);
}

short Score()
{
  short ret;

#if !WWW
  if ((ret = LockScore()))
       return ret;
  if ((ret = ReadScore()) == 0)
    if ((ret = MakeScore()) == 0)
      ret = WriteScore();
  UnlockScore();
#else
  if (!(ret = MakeScore())) ret = WriteScore_WWW();
#endif
  return ((ret == 0) ? E_ENDGAME : ret);
}

void ntohs_entry(struct st_entry *entry)
{
    entry->lv = ntohs(entry->lv);
    entry->mv = ntohs(entry->mv);
    entry->ps = ntohs(entry->ps);
    entry->date = ntohl(entry->date);
}

#if !WWW
static short ReadOldScoreFile01();
#endif
static short ReadScore()
{
#if WWW
    return ReadScore_WWW();
#else
  short ret = 0;
  long tmp;
  struct stat s;

  if (0 > stat(SCOREFILE, &s)) {
    return E_FOPENSCORE;
  }
  date_stamp = s.st_mtime;

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
	if (0 == strcmp(magic, "xs01")) {
	    fprintf(stderr, "Warning: old-style score file\n");
	    return ReadOldScoreFile01();
	} else {
	    fprintf(stderr, "Error: unrecognized score file format. May be"
			    "  obsolete, or else maybe this program is.\n");
	    ret = E_READSCORE;
	    (void)close(ret);
	    return ret;
	}
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

#if !WWW
short ReadOldScoreFile01()
{
    short ret = 0;
    time_t now = time(0);
    if (read(sfdbn, &scoreentries, 2) != 2)
      ret = E_READSCORE;
    else {
      int tmp;
      struct old_st_entry *t = (struct old_st_entry *)malloc(scoreentries *
		sizeof(struct old_st_entry));
      scoreentries = ntohs(scoreentries);
      tmp = scoreentries * sizeof(t[0]);
      if (read(sfdbn, &t[0], tmp) != tmp)
	ret = E_READSCORE;

      /* swap up for little-endian machines */
      for (tmp = 0; tmp < scoreentries; tmp++) {
	scoretable[tmp].lv = ntohs(t[tmp].lv);
	scoretable[tmp].mv = ntohs(t[tmp].mv);
	scoretable[tmp].ps = ntohs(t[tmp].ps);
	strncpy(scoretable[tmp].user, t[tmp].user, MAXUSERNAME);
	scoretable[tmp].date = (int)now;
      }
    }
    (void)close(sfdbn);
    return ret;
}
#endif

int SolnRank(int j, Boolean *ignore)
{
    int i, rank = 1;
    unsigned short level = scoretable[j].lv;
    for (i = 0; i < j; i++) {
	if (VALID_ENTRY(i) &&
	    !(ignore && ignore[i]) && scoretable[i].lv == level) {
	    if (scoretable[i].mv == scoretable[j].mv &&
		scoretable[i].ps == scoretable[j].ps &&
		0 == strcmp(scoretable[i].user, scoretable[j].user)) {
		    rank = BADSOLN;
	    }
	    if ((scoretable[i].mv < scoretable[j].mv &&
		 scoretable[i].ps <= scoretable[j].ps) ||
	        (scoretable[i].mv <= scoretable[j].mv &&
		 scoretable[i].ps < scoretable[j].ps)) {
		if (0 == strcmp(scoretable[i].user, scoretable[j].user))
		    rank = BADSOLN;
		else
		    rank++;
	    }
	}
    }
    return rank;
}

static void DeleteLowRanks()
{
    int i;
    Boolean deletable[MAXSCOREENTRIES];
    for (i = 0; i < scoreentries; i++) {
	deletable[i] = _false_;
	if (SolnRank(i, deletable) > MAXSOLNRANK &&
	    0 != strcmp(scoretable[i].user, username)) {
	      deletable[i] = _true_;
	}
    }
    FlushDeletedScores(deletable);
}

static void CleanupScoreTable()
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

static void FlushDeletedScores(Boolean delete[])
{
    int i, k = 0;
    for (i = 0; i < scoreentries; i++) {
	if (i != k) CopyEntry(k, i);
	if (!delete[i]) k++;
    }
    scoreentries = k;
}

static short MakeScore()
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
  scoretable[pos].date = (int)time(0);
  scoreentries++;

  CleanupScoreTable();
  if (scoreentries == MAXSCOREENTRIES)
    return E_TOMUCHSE;
  else
    return 0;
}

#if !WWW
static short FindUser()
{
  short i;
  Boolean found = _false_;

  for (i = 0; (i < scoreentries) && (!found); i++)
    found = (strcmp(scoretable[i].user, username) == 0);
  return ((found) ? i - 1 : -1);
}
#endif

static short FindPos()
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

#if !WWW

char const *tempnm = SCOREFILE "XXXXXX";

static short WriteScore()
{
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
	scoretable[tmp].date = htonl(scoretable[tmp].date);
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
	scoretable[tmp].date = ntohl(scoretable[tmp].date);
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
}
#endif

#ifdef HAVE_NL_LANGINFO
int mos[] = { ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6,
	      ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12 };
#define MONTH(x) nl_langinfo(mos[x])
#else
char *mos[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
#define MONTH(x) mos[x]
#endif

char date_buf[10];

char *DateToASCII(time_t date)
{
    if (datemode) {
	sprintf(date_buf, "%d", (int)date);
    } else {
	struct tm then, now;
	time_t dnow = time(0);
	now = *localtime(&dnow);
	then = *localtime(&date);
	if (then.tm_year != now.tm_year) {
	    sprintf(date_buf, "%s %d", MONTH(then.tm_mon), then.tm_year % 100);
	} else if (then.tm_mon != now.tm_mon ||
		   then.tm_mday != now.tm_mday) {
	    sprintf(date_buf, "%d %s", then.tm_mday, MONTH(then.tm_mon));
	} else {
	    int hour = then.tm_hour % 12;
	    if (hour == 0) hour = 12;
	    sprintf(date_buf, "%d:%.2d%s", hour, then.tm_min,
				(then.tm_hour < 12) ? "am" : "pm");
	}
    }
    return date_buf;
}

#define TRY(name,expr) do { if (0>(expr)) { perror(name); }} while (0)

static void ShowScoreLine(int i)
{
    int rank = SolnRank(i, 0);
    if (rank <= MAXSOLNRANK) TRY("printf", printf("%4d", rank));
    else TRY("printf", printf("    "));
    TRY("printf",
    fprintf(stdout, " %32s %4d     %4d     %4d   %s\n", scoretable[i].user,
	    scoretable[i].lv, scoretable[i].mv, scoretable[i].ps,
	    DateToASCII((time_t)scoretable[i].date)));
}


static void ShowScore(int level)
{
    int i;
    DEBUG_SERVER("entering ShowScore");
    if (!headermode) {
        TRY("printf",
            printf("Rank                             User  Level   Moves"
	           "  Pushes   Date\n"));
        TRY("printf",
            printf("==========================================="
		   "===========================\n"));
	for (i = 0; i < scoreentries; i++) {
	    if (level == 0 || scoretable[i].lv == level) {
		ShowScoreLine(i);
	    }
	}
    } else {
	int top = -1, bottom = -1;
	DeleteLowRanks();
	if (level == 0) {
	    top = 0;
	    bottom = scoreentries - 1;
	} else {
	    for (i = 0; i < scoreentries; i++) {
		if (scoretable[i].lv == level) {
		    if (top == -1) top = i;
		    bottom = i;
		}
	    }
	}
	DumpLinesWithHeader(top, bottom);
    }
}

static void CopyEntry(short i1, short i2)
{
  strcpy(scoretable[i1].user, scoretable[i2].user);
  scoretable[i1].lv = scoretable[i2].lv;
  scoretable[i1].mv = scoretable[i2].mv;
  scoretable[i1].ps = scoretable[i2].ps;
  scoretable[i1].date = scoretable[i2].date;
}

/* Extract one line from "text".  Return 0 if there is no line to extract. */
char *getline(char *text, char *linebuf, int bufsiz)
{
    if (*text == 0) {
	*linebuf = 0;
	return 0;
    }
    bufsiz--; /* for trailing null */
    while (*text != '\n' && *text != '\r' && *text && bufsiz != 0) {
	*linebuf++ = *text++;
	bufsiz--;
    }
    if (text[0] == '\r' && text[1] == '\n') text++; /* skip over CRLF */
    *linebuf = 0;
    return (*text) ? text + 1 : text ;
	/* point to next line or final null */
}

#if WWW
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

static char *subst_names(char const *template)
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
    if (0 == strcmp(HERE, "@somewhere.somedomain")) {
	fprintf(stderr, "In order to save a score, fix the configuration\n"
			"variable HERE (in config.h) and recompile.\n");
	return E_WRITESCORE;
    }
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
    } else {
	return E_WRITESCORE;
    }
    return 0;
}

char *skip_past_header(char *text)
{
    char line[256];
    do {
	text = getline(text, line, sizeof(line));
	if (!text) return 0;
    } while (0 != strcmp(line, ""));
    return text;
}

/*
    Yes, it actually parses xsokoban's own output format! Pretty
    disgusting!
*/
short ReadScore_WWW()
{
    char *cmd, *text;
    short ret;
    movelist[0] = 0;
    cmd = subst_names(WWWREADSCORECMD);
    text = qtelnet(WWWHOST, WWWPORT, cmd);
/* Now, skip past all the initial crud */
    if (!text) return E_READSCORE;
    ret = ParseScoreText(text, _false_);
    free(text);
    return ret;
}

short GetUserLevel_WWW(short *lv)
{
    char *cmd, *text;
    short ret;
    movelist[0] = 0;
    cmd = subst_names(WWWGETLEVELPATH);
    text = qtelnet(WWWHOST, WWWPORT, cmd);
    free(cmd);
/* Now, skip past all the initial crud */
    if (!text) return E_READSCORE;
    ret = ParseUserLevel(text, lv);
    free(text);
    return ret;
}

static short ParseUserLevel(char *text, short *lv)
{
    char line[256];
    text = skip_past_header(text);
    if (!text) return E_READSCORE;
    text = getline(text, line, sizeof(line));
    if (!text) return E_READSCORE;
    if (0 == strncmp(line, "Level: ", 7)) {
	*lv = atoi(&line[7]);
	return 0;
    } else {
	return E_READSCORE;
    }
}

static void DeleteAllEntries()
{
    int i;
    for (i = 0; i < scoreentries; i++) scoretable[i].user[0] = 0;
}

#define GRAB(tag, stmt) 					\
    text = getline(text, line, sizeof(line)); 			\
    if (!text) return E_READSCORE; 		 		\
    if (0 == strncmp(line, tag, strlen(tag))) { stmt; } 	\
    else return E_READSCORE;
    
short FetchScoreLevel_WWW(int *line1 /*out*/, int *line2 /*out*/)
{
    char *start, *text, *cmd = subst_names(WWWGETSCORELEVELPATH);
    short ret;
    start = text = qtelnet(WWWHOST, WWWPORT, cmd);
    free(cmd);
    if (!text) { free(start); return E_READSCORE; }
    ret = ParsePartialScore(start, line1, line2);
    free(start);
    return ret;
}

short FetchScoreLines_WWW(int *line1 /* in/out */, int *line2 /* int/out */)
{
    char *start, *text, *cmd = subst_names(WWWGETLINESPATH);
    char cmdbuf[256];
    short ret;
    sprintf(cmdbuf, cmd, *line1, *line2);
    start = text = qtelnet(WWWHOST, WWWPORT, cmdbuf);
    free(cmd);
    if (!text) { free(start); return E_READSCORE; }
    ret = ParsePartialScore(start, line1, line2);
    free(start);
    return ret;
}

short ParsePartialScore(char *text, int *line1, int *line2)
{
    short ret = 0;
    Boolean outofdate = _false_;
    char line[256];
    time_t newdate;
    int i;
    text = skip_past_header(text);
    if (!text) return E_READSCORE;
    GRAB("Entries: ", scoreentries = atoi(line + 9));
    GRAB("Line1: ", *line1 = atoi(line + 7));
    GRAB("Line2: ", *line2 = atoi(line + 7));
    GRAB("Date: ", newdate = atoi(line + 6));
    GRAB("==========", );

    if (newdate == 0) return E_READSCORE;
    if (newdate != date_stamp) {
	DeleteAllEntries();
	outofdate = _true_;
    }

    date_stamp = newdate;

    for (i = *line1; i < *line2; i++) {
	if ((ret = ParseScoreLine(scoreentries - i - 1, &text, _true_))
	     || !text)
	{
	    DeleteAllEntries();
	    return E_READSCORE;
	}
    }
    if (ret == 0 && outofdate) return E_OUTOFDATE;
    else return ret;
}
#endif

static short ParseScoreText(char *text, Boolean allusers)
{
    char line[256];
    do {
	text = getline(text, line, sizeof(line));
	if (!text) return E_READSCORE;
    } while (line[0] != '=');
    scoreentries = 0;
    while (text) {
	ParseScoreLine(scoreentries, &text, allusers);
	if (VALID_ENTRY(scoreentries)) scoreentries++;
    }
    return 0;
}

static short ParseScoreLine(int i, char **text /* in out */, Boolean all_users)
{
    char *user, *date_str;
    char *ws = " \t\r\n";
    int level, moves, pushes;
    int date = 0; /* time_t */
    Boolean baddate = _false_;
    int rank;
    char rank_s[4];
    char line[256];
    *text = getline(*text, line, sizeof(line));
    if (!*text) return 0;
    strncpy(rank_s, line, 4);
    rank = atoi(rank_s);
    user = strtok(line + 4, ws);
    if (!user) { *text = 0; return 0; }
    if (all_users || rank != 0 || 0 == strcmp(user, username)) {
	level = atoi(strtok(0, ws)); if (!level) return E_READSCORE;
	moves = atoi(strtok(0, ws)); if (!moves) return E_READSCORE;
	pushes = atoi(strtok(0, ws)); if (!pushes) return E_READSCORE;
	date_str = strtok(0, ws);
	if (date_str) date = (time_t)atoi(date_str);
	if (!date) {
	    date = time(0);
	    if (!baddate) {
		baddate = _true_;
		fprintf(stderr,
			"Warning: Bad or missing date in ASCII scores\n");
	    }
	}
	strncpy(scoretable[i].user, user, MAXUSERNAME);
	scoretable[i].lv = (unsigned short)level;
	scoretable[i].mv = (unsigned short)moves;
	scoretable[i].ps = (unsigned short)pushes;
	scoretable[i].date = date;
    } else {
	scoretable[i].user[0] = 0;
    }
    return 0;
}

int FindCurrent()
/*
    Return the scoretable index pointing to level "level", or as
    close as possible. If the scoretable is empty, return -1.
*/
{
    int i;
    for (i = 0; i < scoreentries; i++) {
	 if (0 == strcmp(scoretable[i].user, username) &&
	     (unsigned short)level == scoretable[i].lv) {
	     return i;
	 }
    }
/* Find largest of smaller-numbered levels */
    for (i = 0; i < scoreentries; i++)
	 if (scoretable[i].user[0] &&
	     (unsigned short)level >= scoretable[i].lv) return i;
/* Find smallest of larger-numbered levels */
    for (i = scoreentries - 1; i >= 0; i--)
	 if (scoretable[i].user[0] &&
	     (unsigned short)level < scoretable[i].lv) return i;
    return -1; /* Couldn't find it at all */
}

