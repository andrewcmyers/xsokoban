#include <stdio.h>
#include <pwd.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "externs.h"
#include "globals.h"
#include "options.h"
#include "errors.h"
#include "display.h"
#include "score.h"
#include "www.h"

/* locally-defined functions */
static short VerifyScore(short optlevel);
static short GameLoop(void);
static short GetGamePassword(void);
static void Error(short);
static void Usage(void);
static short CheckCommandLine(int *, char **);
static char *FixUsername(char *name);

/* exported globals */
Boolean scoring = _true_;
short level, packets, savepack, moves, pushes, rows, cols;
unsigned short scorelevel, scoremoves, scorepushes;
POS ppos;
char map[MAXROW + 1][MAXCOL + 1];
char *username = 0, *progname = 0, *bitpath = 0;
char *optfile = 0;
XrmDatabase rdb;
Boolean ownColormap = _false_, datemode = _false_, headermode = _false_;

static short optlevel = 0, userlevel = 0;
static short line1 = 0, line2 = 0;
static Boolean opt_show_score = _false_, opt_make_score = _false_,
	       optrestore = _false_, owner = _false_,
	       opt_verify = _false_, opt_partial_score = _false_,
	       opt_user_level = _false_;
static struct passwd *pwd;


static int movelen;
/* Length of the verified move sequence waiting on stdin if -v is used */

void main(int argc, char **argv)
{
  short ret = 0;

  DEBUG_SERVER("starting");

  scorelevel = 0;
  moves = pushes = packets = savepack = 0;

  /* make the program name be what it is invoked with */
  progname = strrchr(argv[0], '/');
  if (progname == NULL)
    progname = argv[0];
  else
    progname++;

  /* Parse the command line */
  ret = CheckCommandLine(&argc, argv);

  /* find out who is playing us. (pwd will be kept around in case we need to
   * build the Xresources stuff later.
   */
  pwd = getpwuid(getuid());
  if (pwd == NULL)
    /* we MUST be being played by somebody, sorry */
    ret = E_NOUSER;
  else {
    /* find out who we are. */
#if !WWW
    if (!username) username = FixUsername(pwd->pw_name);
    /* else we already got a fixed username from the -v option */
#else
/* If running in Web mode, append HERE to the username. */
    if (!username) {
/* username might have already been set by the X resource mechanism */
	char *here = HERE;
	char *namebuf = (char *)malloc(strlen(pwd->pw_name) + strlen(here) + 1);
	strcpy(namebuf, pwd->pw_name);
	strcat(namebuf, here);
	username = FixUsername(namebuf);
	free(namebuf);
    } else {
	username = FixUsername(username);
    }
#endif
    /* see if we are the owner */
    owner = (strcmp(username, OWNER) == 0) ? _true_ : _false_;
    if (ret == 0) {
      if (opt_show_score) {
	DEBUG_SERVER("sending score file");
	ret = OutputScore(optlevel);
      } else if (opt_verify) {
	DEBUG_SERVER("verifying score");
	ret = VerifyScore(optlevel);
      } else if (opt_partial_score) {
	ret = OutputScoreLines(line1, line2);
      } else if (opt_make_score) {
	if (owner) {
	  /* make sure of that, shall we? */
	  ret = GetGamePassword();
	  if (ret == 0)
	    ret = MakeNewScore(optfile);
	} else
	  /* sorry, BAD owner */
	  ret = E_NOSUPER;
      } else if (optrestore) {
	ret = RestoreGame();
      } else if (opt_user_level) {
	ret = GetUserLevel(&userlevel);
	if (ret == 0) {
	    printf("Level: %d\n", userlevel);
	    ret = E_ENDGAME;
	}
      } else {
	ret = GetUserLevel(&userlevel);
	if (ret == 0) {
	    if (optlevel > 0) {
#if !ANYLEVEL
		if (userlevel < optlevel) {
		    if (owner) {
			/* owners can play any level (but not score),
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
  if (ret == 0) {
    /* play till we drop, then nuke the good stuff */
    ret = GameLoop();
    DestroyDisplay();
    XCloseDisplay(dpy); /* factored this out to allow re-init */
    display_alloc = _false_;
  }
  /* always report here since the game returns E_ENDGAME when the user quits.
   * Sigh.. it would be so much easier to just do it right.
   */
  Error(ret);

  /* exit with whatever status we ended with */
  switch(ret)
  {
  case E_ENDGAME:
  case E_SAVED:
	ret = 0;	/* normal exits */
	break;
  }
  DEBUG_SERVER("ending");
  exit(ret);
}

/* FixUsername makes sure that the username contains no spaces or
   unprintable characters, and is less than MAXUSERNAME characters
   long. */
static char *FixUsername(char *name)
{
    char namebuf[MAXUSERNAME];
    char *c = namebuf;
    strncpy(namebuf, name, MAXUSERNAME);
    namebuf[MAXUSERNAME-1] = 0;
    while (*c) {
	if (!isprint(*c) || *c == ' ' || *c == ',') *c = '_';
	c++;
    }
    return strdup(namebuf);
}

static Boolean mode_selected()
{
    return (opt_show_score || opt_make_score || optrestore || (optlevel > 0) ||
	     opt_verify || opt_partial_score || opt_user_level)
	   ? _true_ : _false_;
}
	     
/* Oh boy, the fun stuff.. Follow along boys and girls as we parse the command
 * line up into little bitty pieces and merge in all the xdefaults that we
 * need.
 * 
 * May set "username" to some value.
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
    if (argv[option][0] == '-') {
      char *optarg;
      switch(argv[option][1]) {
	case 's':
	  if (mode_selected()) return E_USAGE;
	  opt_show_score = _true_;
	  optarg = &argv[option][2];
	  if (!isdigit(*optarg) && argv[option+1] && isdigit(argv[option+1][0]))
	  {
	    optarg = &argv[option+1][0];
	    option++;
	  }
	  optlevel = atoi(optarg);
	  break;
	case 'c':
	  if (mode_selected()) return E_USAGE;
	  optfile = 0;
	  if (argv[option][2] != 0) 
	    optfile = &argv[option][2];
	  else if (argv[option+1] && argv[option + 1][0] != '-') {
	    optfile = &argv[option + 1][0];
	    option++;
	  }
	  opt_make_score = _true_;
	  break;
	case 'C':
	  ownColormap = _true_;
	  break;
	case 'r':
	  if (mode_selected()) return E_USAGE;
	  optrestore = _true_;
	  break;
	case 'v':
	  if (mode_selected()) return E_USAGE;
	  option++;
	  optlevel = atoi(argv[option++]);
	  if (!optlevel || !argv[option]) return E_USAGE;
	  username = FixUsername(argv[option++]);
	  if (!argv[option]) return E_USAGE;
	  movelen = atoi(argv[option++]);
	  if (!movelen) return E_USAGE;
	  opt_verify = _true_;
	  break;
	case 'L':
	  if (mode_selected()) return E_USAGE;
	  option++;
	  if (!argv[option]) return E_USAGE;
	  line1 = atoi(argv[option++]);
	  if (!argv[option]) return E_USAGE;
	  line2 = atoi(argv[option]);
	  if (line1 > line2) return E_USAGE;
	  opt_partial_score = _true_;
	  break;
	case 'u':
	  option++;
	  if (!argv[option]) return E_USAGE;
	  username = FixUsername(argv[option]);
	  break;
	case 'U':
	  if (mode_selected()) return E_USAGE;
	  opt_user_level = _true_;
	  break;
	case 'D':
	  datemode = _true_;
	  break;
	case 'H':
	  headermode = _true_;
	  break;
	default:
	  if (mode_selected()) return E_USAGE;
	  optlevel = atoi(argv[option]+1);
	  if (optlevel == 0) return E_USAGE;
	  break;
      }
    } else
      /* found an option that didn't begin with a - (oops) */
      return E_USAGE;
  }

  if (opt_partial_score || opt_show_score || opt_make_score || opt_verify ||
      opt_user_level)
      return 0; /* Don't mess with X any more */
  /* okay.. NOW, find out what display we are currently attached to. This
   * allows us to put the display on another machine
   */
  res = GetDatabaseResource(command, "display");

  /* open up the display */
  dpy = XOpenDisplay(res);
  if (dpy == (Display *)NULL)
    return E_NODISPLAY;
  display_alloc = _true_;

  /* okay, we have a display, now we can get the std xdefaults and stuff */
  res = XResourceManagerString(dpy);
  if (res != NULL)
    /* try to get it off the server first (ya gotta love R4) */
    rdb = XrmGetStringDatabase(res);
  else {
    /* can't get it from the server, let's do it the slow way */
    /* try HOME first in case you have people sharing accounts :) */
    res = getenv("HOME");
    if (res != NULL)
      strcpy(buf, res);
    else
      /* no HOME, let's try and make one from the pwd (whee) */
      strcpy(buf, pwd->pw_dir);
    strcat(buf, "/.Xdefaults");
    rdb = XrmGetFileDatabase(buf);
  }

  /* let's merge in the X environment */
  res = getenv("XENVIRONMENT");
  if (res != NULL) {
    temp = XrmGetFileDatabase(res);
    XrmMergeDatabases(temp, &rdb);
  }

  /* now merge in the rest of the X command line options! */
  XrmMergeDatabases(command, &rdb);

  if (!username) username = GetResource("username");
  return 0;
}

/* Read a move sequence from stdin in a newly-allocated string. */
static char *ReadMoveSeq()
{
    char *moveseq = (char *)malloc(movelen);
    int ch = 0;
    while (ch < movelen) {
	int n = read(0, moveseq + ch, movelen - ch); /* read from stdin */
	if (n <= 0) { perror("Move sequence"); return 0; }
	ch += n;
    }
    return moveseq;
}
  
short VerifyScore(short optlevel)
{
    short ret;
    char *moveseq = ReadMoveSeq();
    if (!moveseq) { return E_WRITESCORE; }
    level = optlevel;
    ret = ReadScreen();
    if (ret) return ret;
    if (Verify(movelen, moveseq)) {
	ret = Score(_false_);
	scorelevel = 0; /* don't score again */
	if (ret == 0) ret = E_ENDGAME;
    } else {
	ret = E_WRITESCORE;
    }
    free(moveseq);
    return ret;
}

/* we just sit here and keep playing level after level after level after .. */
static short GameLoop(void)
{
    short ret = 0;
    
    /* make sure X is all set up and ready for us */
    ret = InitX();
    if (ret == E_NOCOLOR && !ownColormap) {
	DestroyDisplay();
	ownColormap = _true_;
	fprintf(stderr,
	"xsokoban: Couldn't allocate enough colors, trying own colormap\n");
	ret = InitX();
    }
    
    if (ret != 0) return ret;
    
    /* get where we are starting from */
    if (!optrestore) ret = ReadScreen();
    
    /* until we quit or get an error, just keep on going. */
    while(ret == 0) {
	ret = Play();
	if ((scorelevel > 0) && scoring) {
	    int ret2;
	    ret2 = Score(_false_);
	    Error(ret2);
	    scorelevel = 0;
	}
	if (ret == 0 || ret == E_ABORTLEVEL) {
	    short newlev = 0;
	    short ret2;
	    ret2 = DisplayScores(&newlev);
	    if (ret2 == 0) {
		if (newlev > 0 &&
#if !ANYLEVEL
		    newlev <= userlevel &&
#endif
		    1) {
		    level = newlev;
		} else {
		    if (ret == 0) level++;
		}
		ret = 0;
	    } else {
		ret = ret2;
	    }
		
	}
	if (ret == 0) {
	    moves = pushes = packets = savepack = 0;
	    ret = ReadScreen();
	}
    }
    return ret;
}

/* Does this really need a comment :) */
static short GetGamePassword(void)
{
  return ((strcmp(getpass("Password: "), PASSWORD) == 0) ? 0 : E_ILLPASSWORD);
}

/* display the correct error message based on the error number given us. 
 * There are 2 special cases, E_ENDGAME (in which case we don't WANT a 
 * silly error message cause it's not really an error, and E_USAGE, in which
 * case we want to give a really nice list of all the legal options.
 */
static void Error(short err)
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
      if (err == E_USAGE)
        Usage();
      break;
    default:
      if (err != E_ENDGAME && err != E_ABORTLEVEL)
	fprintf(stderr, "%s: %s\n", progname, errmess[0]);
      break;
  }
}

/* this simply prints out the usage string nicely. */
static void Usage(void)
{
  short i;

  fprintf(stderr, USAGESTR, progname);
  for (i = 0; usages[i] != NULL; i++)
    fprintf(stderr, "%s", usages[i]);
}
