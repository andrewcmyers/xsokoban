#ifndef _SCORE_H
#define _SCORE_H

extern short OutputScore(int level);
/* Print out the score list for level "level". If "level" == 0, show
   scores for all levels.
*/

extern short MakeNewScore(char *textfile);
/* Create a new score file. If "textfile" is non-zero, read the
   scores out of the text file of that name. */

extern short GetUserLevel(short *);
/* Get the player's current level based on the level they last scored on.
   Return 1 if the player has no score in the file. */

extern short Score();
/* Add the current score to the score file. */

#define BADSOLN 100

extern int SolnRank(int j, Boolean *ignore);
/* Return the solution rank for table index "j". The solution rank for
   an entry is one greater than the number of entries that are better
   than it, unless there is a better or equal solution that is by the
   same person, in which case the solution rank returned is at least "BADSOLN".
   If two solutions are equal, the one that was arrived at first, and
   thus has a lower table index, is considered to be better.
   One solution is at least as good as another solution if it is at
   least as good in numbers of moves and pushes. Note that
   non-comparable solutions may exist.

   The array "ignore" indicates that some scoretable entries should
   be ignored for the purpose of computing rank.
*/

extern char *DateToASCII(time_t date);
/* Produce an ASCII representation of this date, returning a pointer to
   a static buffer of characters. The representations will be at most
   9 characters long. */

extern short OutputScoreLines(int line1, int line2);
/* 
   Print the scorefile lines from line1 up to line2 - 1. Print the
   total number of lines in the file and the datestamp too (as a
   time_t), followed by a line of at least 40 equal signs. Note that
   scorefile line indices are in reverse order as considered by this
   procedure: the last line in the file is 0.  The scorefile lines will
   be printed in the reverse order that they are printed by
   "OutputScore".

   "line1" must be greater than or equal to 0. If it is greater or
   equal to the number of entries in the file, then it will be treated
   as though it were equal to the number of entries in the file.

   "line2" must be greater than or equal to line1. If "line2" is greater
   than the number of entries in the score file, the "Line2:" output
   field will contain the number of entries in the file.


   Entries: nnnn
   Line1: nnnn
   Line2: nnnn
   Date: ddddddd
   ========================================...
   <scorefile lines>
*/

short FetchScoreLevel_WWW(int *line1 /* out */, int *line2 /* out */);
short FetchScoreLines_WWW(int *line1 /* in/out */, int *line2 /* in/out */);
/*
    Fetch lines of the score file from the remote server. The lines
    requested are placed in "*line1" and "*line2"; the actual lines
    fetched are placed in "*line1" and "*line2".

    Returns E_OUTOFDATE if the scorefile has been modified since the
    last read. Returns E_READSCORE if the scorefile is mangled or the
    server cannot be accessed.
*/

int FindCurrent();
/*
    Return the scoretable index pointing to level "level"
*/

#define VALID_ENTRY(i) (0 != scoretable[i].user[0])
/*
    Report whether entry "i" is currently cached here. Always true for
    non-WWW mode.
*/

/******************************************
 Private to score.c and scoredisp.c
*/

extern short scoreentries;

struct old_st_entry {
  char user[MAXUSERNAME];
  unsigned short lv, pad1, mv, pad2, ps, pad3;
};

extern struct st_entry {
  char user[MAXUSERNAME]; /* If "", this is an empty entry */
  unsigned short lv, pad1, mv, pad2, ps, pad3;
  int date; /* really a time_t */
} scoretable[MAXSCOREENTRIES];

#endif /* _SCORE_H */
