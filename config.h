/***********************************************************************
   Configuration variables for xsokoban

   You may wish to alter the directory paths, particularly ROOTDIR.

   Note that the string concatenation performed below requires an
   ANSI-standard preprocessor. If you don't have one, you'll have to
   manually edit SCREENPATH, SAVEPATH, etc. to start with ROOTDIR.
***********************************************************************/

/*
   ROOTDIR: The directory below which the xsokoban data files, such as
	    the bitmaps, score file, and saved games, are stored.  I
	    suggest "/usr/local/lib/xsokoban" as a value for this
	    variable in the installed version, but you know best...
*/
#ifndef ROOTDIR
#define ROOTDIR "."
#endif

/*
   SCREENPATH: the name of the directory where the screen files are held
*/
#ifndef SCREENPATH
#define SCREENPATH ROOTDIR "/screens"
#endif

/*
   SAVEPATH: the name of the path where save files are held
             Attention: Be sure that there are no other files with
                        the name <username>.sav
*/
#ifndef SAVEPATH
#define SAVEPATH ROOTDIR "/saves"
#endif

/*
   BITPATH: the full pathname to the bitmap file defaults. If you want
	    to support both color and B&W displays, change this to
	    ROOTDIR "/bitmaps/defaults" and create symbolic links in
	    that directory to all the files in bitmaps/color and
	    bitmaps/bw. The program will automatically use the bitmaps
	    if the display isn't 8 bits deep.
*/
#ifndef BITPATH
#define BITPATH ROOTDIR "/bitmaps/defaults"
#endif

/*
   LOCKPATH: temporary file which is created to ensure that no users
             work with the scorefile at the same time. It should be
             in the same directory as the score file.
*/
#ifndef LOCKFILE
#define LOCKFILE ROOTDIR "/scores/lock"
#endif

/*
   SCOREFILE: the full pathname of the score file
*/
#ifndef SCOREFILE
#define SCOREFILE ROOTDIR "/scores/scores"
#endif

/*
   USE_XPM: Look for color pixmaps to define the appearance of the
	    game.  This requires that you have the XPM library, which
	    reads in ".xpm" files and produces pixmaps.
*/
#ifndef USE_XPM
#define USE_XPM TRUE
#endif

/*
   MAXUSERNAME: defines the maximum length of a system's user name
*/
#define MAXUSERNAME     32

/*
   MAXSCOREENTRIES: defines the maximum number of entries in the scoretable
*/
#define MAXSCOREENTRIES 10000

/*
   SUPERUSER: defines the name of the local game owner. Almost never "root"!
*/
#ifndef SUPERUSER
#define SUPERUSER "andru"
#endif

/*
   PASSWORD: defines the password necessary for creating a new score file
*/
#ifndef PASSWORD
#define PASSWORD "score"
#endif

/*
   ANYLEVEL: Allow any user to play any level and enter a score for it
*/
#define ANYLEVEL FALSE

/*
   MAXSOLNRANK: The maximum solution rank for which an entry is retained
   in the score table.
*/
#define MAXSOLNRANK 5

/*
   STACKDEPTH: Number of previous positions remembered in the move stack
*/
#define STACKDEPTH 1000

/*
   TIMEOUT: How long a lock can be held on the score file, in seconds
*/
#define TIMEOUT 10
