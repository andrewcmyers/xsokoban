#include <stdio.h>
#include <pwd.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "externs.h"
#include "globals.h"
#include "options.h"
#include "errors.h"

/* useful globals */
Boolean scoring = _true_;
short level, packets, savepack, moves, pushes, rows, cols;
unsigned short scorelevel, scoremoves, scorepushes;
POS ppos;
char map[MAXROW + 1][MAXCOL + 1];
char *username = NULL, *progname = NULL, *bitpath = NULL;
XrmDatabase rdb;

static short optlevel = 0, userlevel;
static Boolean optshowscore = _false_, optmakescore = _false_,
	       optrestore = _false_, superuser = _false_;
static struct passwd *pwd;

/* do all the setup foo, and make sure command line gets parsed. */
void main(int argc, char **argv)
{
  short ret = 0, ret2 = 0;

#ifdef VICE
  Authenticate();
#endif

  scorelevel = 0;
  moves = pushes = packets = savepack = 0;

  /* make the program name be what it is invoked with */
  progname = strrchr(argv[0], '/');
  if(progname == NULL)
    progname = argv[0];
  else
    progname++;

  /* find out who is playing us. (pwd will be kept around in case we need to
   * build the Xresources stuff later.
   */
  pwd = getpwuid(getuid());
  if(pwd == NULL)
    /* we MUST be being played by somebody, sorry */
    ret = E_NOUSER;
  else {
    /* find out who we are. */
    username = pwd->pw_name;
    /* see if we are the superuser */
    superuser = (strcmp(username, SUPERUSER) == 0) ? _true_ : _false_;
    /* Parse the command line */
    ret = CheckCommandLine(&argc, argv);
    if(ret == 0) {
      if(optshowscore)
	ret = OutputScore();
      else if(optmakescore) {
	if(superuser) {
	  /* make sure of that, shall we? */
	  ret = GetGamePassword();
	  if(ret == 0)
	    ret = MakeNewScore();
	} else
	  /* sorry, BAD superuser */
	  ret = E_NOSUPER;
      } else if(optrestore) {
	ret = RestoreGame();
      } else {
	ret = GetUserLevel(&userlevel);
	if(ret == 0) {
	    if(optlevel > 0) {
#ifndef ANYLEVEL
		if (userlevel < optlevel) {
		    if (superuser) {
			/* superusers can play any level (but not score),
			 * which is useful for testing out new boards.
			 */
			level = optlevel;
			scoring = _false_;
		    } else {
			ret = E_LEVELTOOHIGH;
		    }
		} else
#endif
		  level = optlevel;
	  } else
	    level = userlevel;
	}
      }
    }
  }
  if(ret == 0) {
    /* play till we drop, then nuke the good stuff */
    ret = GameLoop();
    DestroyDisplay();
  }
  /* always report here since the game returns E_ENDGAME when the user quits.
   * Sigh.. it would be so much easier to just do it right.
   */
  Error(ret);
  /* see if they score, and do it (again report an error */
  if((scorelevel > 0) && scoring) {
    ret2 = Score(_true_);
    Error(ret2);
  }
  /* exit with whatever status we ended with */
  exit(ret);
}

/* Oh boy, the fun stuff.. Follow along boys and girls as we parse the command
 * line up into little bitty pieces and merge in all the xdefaults that we
 * need.
 */
short CheckCommandLine(int *argcP, char **argv)
{
  XrmDatabase command = NULL, temp = NULL;
  char *res;
  char buf[1024];
  int option;

  /* let's do this the sensible way, Command line first! */
  /* we will also OPEN the display here, though we won't do anything with it */
  XrmInitialize();

  /* build an XrmDB from the command line based on the options (options.h) */
  XrmParseCommand(&command, options, sizeof(options)/sizeof(*options),
		  progname, argcP, argv);

  /* okay, we now have the X command line options parsed, we might as well
   * make sure we need to go further before we do.  These command line options
   * are NOT caught by XrmParseCommand(), so we need to do them ourselves.
   * Remember, they are all exclusive of one another.
   */
  for(option = 1; option < *argcP; option++) {
    if(argv[option][0] == '-') {
      switch(argv[option][1]) {
	case 's':
	  if(optshowscore || optmakescore || optrestore || (optlevel > 0))
	    return E_USAGE;
	  optshowscore = _true_;
	  break;
	case 'c':
	  if(optshowscore || optmakescore || optrestore || (optlevel > 0))
	    return E_USAGE;
	  optmakescore = _true_;
	  break;
	case 'r':
	  if(optshowscore || optmakescore || optrestore || (optlevel > 0))
	    return E_USAGE;
	  optrestore = _true_;
	  break;
	default:
	  if(optshowscore || optrestore || optmakescore || (optlevel > 0))
	    return E_USAGE;
	  optlevel = atoi(argv[option]+1);
	  if(optlevel == 0)
	    return E_USAGE;
	  break;
      }
    } else
      /* found an option that didn't begin with a - (oops) */
      return E_USAGE;
  }

  if (optshowscore) return 0; /* Don't mess with X any more */
  /* okay.. NOW, find out what display we are currently attached to. This
   * allows us to put the display on another machine
   */
  res = GetDatabaseResource(command, "display");

  /* open up the display */
  dpy = XOpenDisplay(res);
  if(dpy == (Display *)NULL)
    return E_NODISPLAY;
  display_alloc = _true_;

  /* okay, we have a display, now we can get the std xdefaults and stuff */
  res = XResourceManagerString(dpy);
  if(res != NULL)
    /* try to get it off the server first (ya gotta love R4) */
    rdb = XrmGetStringDatabase(res);
  else {
    /* can't get it from the server, let's do it the slow way */
    /* try HOME first in case you have people sharing accounts :) */
    res = getenv("HOME");
    if(res != NULL)
      strcpy(buf, res);
    else
      /* no HOME, let's try and make one from the pwd (whee) */
      strcpy(buf, pwd->pw_dir);
    strcat(buf, "/.Xdefaults");
    rdb = XrmGetFileDatabase(buf);
  }

  /* let's merge in the X environment */
  res = getenv("XENVIRONMENT");
  if(res != NULL) {
    temp = XrmGetFileDatabase(res);
    XrmMergeDatabases(temp, &rdb);
  }

  /* now merge in the rest of the X command line options! */
  XrmMergeDatabases(command, &rdb);
  return 0;
}

/* we just sit here and keep playing level after level after level after .. */
short GameLoop(void)
{
  short ret = 0;

  /* make sure X is all set up and ready for us */
  ret = InitX();
  if(ret != 0)
    return ret;

  /* get where we are starting from */
  if(!optrestore)
    ret = ReadScreen();

  /* until we quit or get an error, just keep on going. */
  while(ret == 0) {
    ret = Play();
    if((scorelevel > 0) && scoring) {
      int ret2;
      ret2 = Score(_false_);
      Error(ret2);
      scorelevel = 0;
    }
    if(ret == 0) {
      level++;
      moves = pushes = packets = savepack = 0;
      ret = ReadScreen();
    }
  }
  return ret;
}

/* Does this really need a comment :) */
short GetGamePassword(void)
{
  return ((strcmp(getpass("Password: "), PASSWORD) == 0) ? 0 : E_ILLPASSWORD);
}

/* display the correct error message based on the error number given us. 
 * There are 2 special cases, E_ENDGAME (in which case we don't WANT a 
 * silly error message cause it's not really an error, and E_USAGE, in which
 * case we want to give a really nice list of all the legal options.
 */
void Error(short err)
{
  switch(err) {
    case E_FOPENSCREEN:
    case E_PLAYPOS1:
    case E_ILLCHAR:
    case E_PLAYPOS2:
    case E_TOMUCHROWS:
    case E_TOMUCHCOLS:
    case E_NOUSER:
    case E_FOPENSAVE:
    case E_WRITESAVE:
    case E_STATSAVE:
    case E_READSAVE:
    case E_ALTERSAVE:
    case E_SAVED:
    case E_TOMUCHSE:
    case E_FOPENSCORE:
    case E_READSCORE:
    case E_WRITESCORE:
    case E_USAGE:
    case E_ILLPASSWORD:
    case E_LEVELTOOHIGH:
    case E_NOSUPER:
    case E_NOSAVEFILE:
    case E_NOBITMAP:
    case E_NODISPLAY:
    case E_NOFONT:
    case E_NOMEM:
    case E_NOCOLOR:
      fprintf(stderr, "%s: %s\n", progname, errmess[err]);
      if(err == E_USAGE)
        Usage();
      break;
    default:
      if(err != E_ENDGAME)
	fprintf(stderr, "%s: %s\n", progname, errmess[0]);
      break;
  }
}

/* this simply prints out the usage string nicely. */
void Usage(void)
{
  short i;

  fprintf(stderr, USAGESTR, progname);
  for (i = 0; usages[i] != NULL; i++)
    fprintf(stderr, "%s", usages[i]);
}
