/***********************************************************************
   Configuration variables for xsokoban

   You may wish to alter the directory paths, particularly ROOTDIR.

   Note that the string concatenation performed below requires an
   ANSI-standard preprocessor. If you don't have one, you'll have to
   manually edit SCREENPATH, SAVEPATH, etc. to start with ROOTDIR.
***********************************************************************/

/* HERE: Your local domain. This string will be appended to every user
   name that is sent to the WWW sokoban server, in order to avoid collisions.
   Change it!

   For example, if you are at Stanford, a good value for HERE would be
   "@stanford.edu". Making HERE specific to individual machines is a
   bad idea.

   Usernames that are specified through the "xsokoban.username" resource
   do not have HERE appended to them.
*/
#ifndef HERE
#define HERE "@somewhere.somedomain"
#endif

/*
   ROOTDIR: The directory below which the xsokoban data files, such as
	    the bitmaps, score file, and saved games, are stored.  I
	    suggest "/usr/local/share/xsokoban" as a value for this
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
   BITPATH: the full pathname to the bitmap/pixmap file defaults.
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
#define USE_XPM 1
#endif

/*
   MAXUSERNAME: defines the maximum length of a system's user name
*/
#define MAXUSERNAME     32

/*
   MAXSCOREENTRIES: defines the maximum number of entries in the scoretable
*/
#define MAXSCOREENTRIES 20000

/*
   OWNER: defines the name of the local game owner.
*/
#ifndef OWNER
#define OWNER "andru"
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
#define ANYLEVEL 0

/*
   MAXSOLNRANK: The maximum solution rank for which an entry is retained
   in the score table.
*/
#define MAXSOLNRANK 10

/*
   STACKDEPTH: Number of previous positions remembered in the move stack
*/
#define STACKDEPTH 1000

/*
   TIMEOUT: How long a lock can be held on the score file, in seconds
*/
#define TIMEOUT 10

/*
   SLEEPLEN: Amount of time to sleep for between moves of the man,
   in msec.
*/
#define SLEEPLEN 8

/*
   WWW: Use WWW to store the score file and screens for you.

   In "www.h", you will find definitions that will allow xsokoban to connect
   to an public xsokoban server maintained by Andrew Myers. The xsokoban
   home page is at
    
       http://xsokoban.lcs.mit.edu/xsokoban.html
    
   In order to create your own WWW xsokoban score server, a few small
   shell scripts must be used; they are not provided in this
   distribution, but can be obtained on request from andru@lcs.mit.edu.
*/

#ifndef WWW
#define WWW 1
#endif

#define DEBUG_SERVER(x)
