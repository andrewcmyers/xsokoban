/***********************************************************************
   You may wish to alter the following directory paths
***********************************************************************/
/**/
/* SCREENPATH: the name of the directioy where the screen file are held */
/**/
#define SCREENPATH "/usr/tmp/xsokoban.v2/screens"

/**/
/* SAVEPATH: the name of the path where save files are held */
/*           Attention: Be sure that there are no other files with */
/*                      the name <username>.sav                    */
/**/
#define SAVEPATH "/usr/tmp/xsokoban.v2/saves"

/* BITPATH: the full pathname to the bitmap file defaults. */
#define BITPATH "/usr/tmp/xsokoban.v2/bitmaps/defaults"

/**/
/* LOCKPATH: temporary file which is created to ensure that no users */
/*           work with the scorefile at the same time                */
/**/
#define LOCKFILE "/tmp/score.slock"

/**/
/* SCOREFILE: the full pathname of the score file */
/**/
#define SCOREFILE "/usr/tmp/xsokoban.v2/sokoban.slock"

/**/
/* MAXUSERNAME: defines the maximum length of a system's user name */
/**/
#define MAXUSERNAME     32

/**/
/* MAXSCOREENTRIES: defines the maximum numner of entries in the scoretable */
/**/
#define MAXSCOREENTRIES 10000

/**/
/* SUPERUSER: defines the name of the game superuser */
/**/
#define SUPERUSER "jt1o"

/**/
/* PASSWORD: defines the password necessary for creating a new score file */
/**/
#define PASSWORD "Greezooble"
