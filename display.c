#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config_local.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#ifdef HAVE_SYS_LIMITS_H
#include <sys/limits.h>
#endif
#endif

#include "externs.h"
#include "globals.h"
#include "defaults.h"
#include "help.h"
#include "display.h"

#if USE_XPM
#include "xpm.h"
#endif

static short LoadBitmaps(void);
static short LoadOneBitmap(char *fname, char *altname, Pixmap *pix, int map);
static void MakeHelpWindows(void);
static Pixmap GetObjectPixmap(int, int, char);
static int PickWall(int, int);
static void DrawString(int, int, char *);
static void DisplayLevel(void);
static void DisplayPackets(void);
static void DisplayHelp(void);

/* mnemonic defines to help orient some of the text/line drawing, sizes */
#define HELPLINE ((bit_height * MAXROW) + 30)
#define STATUSLINE ((bit_height * MAXROW) + 5)
#define HELP_H (bit_height * MAXROW)
#define HELP_W (bit_width * MAXCOL)

/* local to this file */
static Window win;
static GC gc, rgc, drgc;
static unsigned int width, height, depth;
static XFontStruct *finfo;
static Boolean optwalls;
static Cursor this_curs;
static Pixmap help[HELP_PAGES], floor;
static Pixmap blank, work, man, saveman, goal, object,
       treasure, walls[NUM_WALLS];
static Boolean font_alloc = _false_, gc_alloc = _false_,
        pix_alloc = _false_, cmap_alloc = _false_, win_alloc = _false_;
static int hlpscrn = -1;
static char buf[500];
static int scr;

/* globals */
Display *dpy;
Atom wm_delete_window, wm_protocols;
Boolean display_alloc = _false_;
unsigned bit_width, bit_height;
Colormap cmap;

/* names of the fancy wall bitmap files.  If you define a set of fancy
 * wall bitmaps, they must use these names
 */
static char *wallname[] = {
 "lonewall", "southwall", "westwall", "llcornerwall",
 "northwall", "vertiwall", "ulcornerwall", "west_twall",
 "eastwall", "lrcornerwall", "horizwall", "south_twall",
 "urcornerwall", "east_twall", "north_twall", "centerwall"
};

/* Do all the nasty X stuff like making the windows, setting all the defaults,
 * creating all the pixmaps, loading everything, and mapping the window.
 * This does NOT do the XOpenDisplay(), so that the -display switch can be
 * handled cleanly.
 */
short InitX(void)
{
  int i;
  Boolean reverse = _false_, tmpwalls = _false_;
  char *rval;
  unsigned long fore, back, bord, curs, gc_mask;
  XSizeHints szh;
  XWMHints wmh;
  XSetWindowAttributes wattr;
  XClassHint clh;
  XGCValues val, reval;
  XTextProperty wname, iname;
  XColor cfg, cbg;
  Atom protocols[1];
  Window root;

  /* these are always needed */
  scr = DefaultScreen(dpy);
  cmap = DefaultColormap(dpy, scr);
  depth = DefaultDepth(dpy, scr);
  root = RootWindow(dpy, scr);

  /* Get a new colormap now, if needed. */
  if (ownColormap) {
      XWindowAttributes wa;
      XGetWindowAttributes(dpy, root, &wa);
      cmap = XCreateColormap(dpy, root, wa.visual, AllocNone);
      cmap_alloc = _true_;
  }
  
  /* here is where we figure out the resources and set the defaults.
   * resources can be either on the command line, or in your .Xdefaults/
   * .Xresources files.  They are read in and parsed in main.c, but used
   * here.
   */
  finfo = GetFontResource(FONT);
  if (!finfo) return E_NOFONT;
  font_alloc = _true_;

  rval = GetResource(REVERSE);
  if(rval != (char *)0) {
    reverse = StringToBoolean(rval);
  }

  fore = GetColorOrDefault(dpy, FOREG, depth, "black", _false_);
  back = GetColorOrDefault(dpy, BACKG, depth, "grey90", _true_);

  if(reverse) {
    unsigned long t;
    t = fore;
    fore = back;
    back = t;
  }

  if(!GetColorResource(BORDER, &bord))
    bord = fore;
  if(!GetColorResource(CURSOR, &curs))
    curs = fore;

  bitpath = GetResource(BITDIR);
  rval = GetResource(WALLS);
  if(rval != (char *)0)
    tmpwalls = StringToBoolean(rval);

  /* Walls are funny.  If a alternate bitpath has been defined, assume
   * !fancywalls unless explicitly told fancy walls.  If the default 
   * bitpath is being used, you can assume fancy walls.
   */
  if(bitpath && !tmpwalls)
    optwalls = _false_;
  else
    optwalls = _true_;

  width = MAXCOL * DEF_BITW;
  height = MAXROW * DEF_BITH + 50;

  wmh.initial_state = NormalState;
  wmh.input = True;
  wmh.flags = (StateHint | InputHint);

  clh.res_class = clh.res_name = progname;

  /* Make sure the window and icon names are set */
  if(!XStringListToTextProperty(&progname, 1, &wname))
    return E_NOMEM;
  if(!XStringListToTextProperty(&progname, 1, &iname))
    return E_NOMEM;

  
  /* load in a cursor, and recolor it so it looks pretty */
  this_curs = XCreateFontCursor(dpy, DEF_CURSOR);
  cfg.pixel = curs;
  cbg.pixel = back;
  XQueryColor(dpy, cmap, &cfg);
  XQueryColor(dpy, cmap, &cbg);
  XRecolorCursor(dpy, this_curs, &cfg, &cbg);

  /* set up the funky little window attributes */
  wattr.background_pixel = back;
  wattr.border_pixel = bord;
  wattr.backing_store = Always;
  wattr.event_mask = (KeyPressMask | ExposureMask | ButtonPressMask |
		      ButtonReleaseMask);
  wattr.cursor = this_curs;
  wattr.colormap = cmap;

  /* Create the window. we create it with NO size so that we
   * can load in the bitmaps; we later resize it correctly.
   */
  win = XCreateWindow(dpy, root, 0, 0, width, height, 4,
		      CopyFromParent, InputOutput, CopyFromParent,
                      (CWBackPixel | CWBorderPixel | CWBackingStore |
                       CWEventMask | CWCursor | CWColormap), &wattr);
  win_alloc = _true_;


  /* this will set the bit_width and bit_height as well as loading
   * in the pretty little bitmaps
   */
  switch (LoadBitmaps()) {
    case 0: break;
    case E_NOCOLOR: return E_NOCOLOR;
    case E_NOBITMAP: return E_NOBITMAP;
  }
  blank = XCreatePixmap(dpy, win, bit_width, bit_height, 1);
  pix_alloc = _true_;

  width = MAXCOL * bit_width;
  height = MAXROW * bit_height + 50;
  
  /* whee, resize the window with the correct size now that we know it */
  XResizeWindow(dpy, win, width, height);

  /* set up the size hints, we don't want manual resizing allowed. */
  szh.min_width = szh.width = szh.max_width = width;
  szh.min_height = szh.height = szh.max_height = height;
  szh.x = szh.y = 0;
  szh.flags = (PSize | PPosition | PMinSize | PMaxSize);

  /* now SET all those hints we create above */
  XSetWMNormalHints(dpy, win, &szh);
  XSetWMHints(dpy, win, &wmh);
  XSetClassHint(dpy, win, &clh);
  XSetWMName(dpy, win, &wname);
  XSetWMIconName(dpy, win, &iname);

  /* Turn on WM_DELETE_WINDOW */
  wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
  wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", 0);
  protocols[0] = wm_delete_window;
  XSetWMProtocols(dpy, win, protocols, 1);

  work = XCreatePixmap(dpy, win, width, height, depth);

  /* set up all the relevant GC's */
  val.foreground = reval.background = fore;
  val.background = reval.foreground = back;
  val.function = reval.function = GXcopy;
  val.font = reval.font = finfo->fid;
  gc_mask = (GCForeground | GCBackground | GCFunction | GCFont);
  gc = XCreateGC(dpy, work, gc_mask, &val);
  rgc = XCreateGC(dpy, blank, gc_mask, &reval);
  drgc = XCreateGC(dpy, work, gc_mask, &reval);

  /* make the help windows and the working bitmaps */
  /* we need to do this down here since it requires GCs to be allocated */
  for(i = 0; i < HELP_PAGES; i++)
    help[i] = XCreatePixmap(dpy, win, HELP_W, HELP_H, depth);
  MakeHelpWindows();
  XFillRectangle(dpy, blank, rgc, 0, 0, bit_width, bit_height);

  gc_alloc = _true_;

  /* display the friendly little clear screen */
  ClearScreen();
  XMapWindow(dpy, win);
  RedisplayScreen();
  
  return 0;
}

/* deallocate all the memory and structures used in creating stuff */
void DestroyDisplay(void)
{
  int i;

  if (!display_alloc) return;

  /* kill the font */
  if (font_alloc) {
      XFreeFont(dpy, finfo);
      font_alloc = _false_;
  }

  if (cmap_alloc) {
      XFreeColormap(dpy, cmap);
      cmap = DefaultColormap(dpy, scr);
      cmap_alloc = _false_;
  }

  /* Destroy everything allocated right around the gcs.  Help windows are
   * freed here cause they are created about the same time.  (Yes, I know
   * this could cause problems, it hasn't yet.
   */
  if(gc_alloc) {
    XFreeGC(dpy, gc);
    XFreeGC(dpy, rgc);
    XFreeGC(dpy, drgc);
    XFreePixmap(dpy, work);
    for (i = 0; i < HELP_PAGES; i++)
      XFreePixmap(dpy, help[i]);
    gc_alloc = _false_;
  }
  /* free up all the allocated pix */
  if(pix_alloc) {
    XFreePixmap(dpy, man);
    XFreePixmap(dpy, saveman);
    XFreePixmap(dpy, goal);
    XFreePixmap(dpy, treasure);
    XFreePixmap(dpy, object);
    XFreePixmap(dpy, floor);
    XFreePixmap(dpy, blank);
    for(i = 0; i < NUM_WALLS; i++)
      if(i == 0 || optwalls)
        XFreePixmap(dpy, walls[i]);
    pix_alloc = _false_;
  }
  /* okay.. NOW we can destroy the main window and the display */
    if (win_alloc) {
	XDestroyWindow(dpy, win);
	win_alloc = _false_;
    }
}

static Boolean full_pixmap[256];

static Boolean TryBitmapFile(char *template, Pixmap *pix, char *bitpath,
			     char *fname, int map)
{
  unsigned int width, height;
  int dum3, dum4;
  sprintf(buf, template, bitpath, fname);

  if(XReadBitmapFile(dpy, win, buf, &width, &height, pix, &dum3, &dum4) ==
       BitmapSuccess) {
       if (width > bit_width) bit_width = width;
       if (height > bit_height) bit_height = height;
       full_pixmap[map] = _false_;
       return _true_;
  }
  return _false_;
}

#if USE_XPM
/*
   Try to load an XPM file. Return 0 if success, E_NOCOLOR if there were
   not enough colors to allocate for the pixmap, E_NOBITMAP if the pixmap
   couldn't be loaded.
*/
static Boolean xpm_color_failed = _false_;

static short TryPixmapFile(char *template, Pixmap *pix, char *bitpath,
                             char *fname, int map)
{
    int ret;
    char *errmsg;
    XpmAttributes attr;
    XWindowAttributes wa;
    if (xpm_color_failed)
	return E_NOCOLOR; /* Don't keep trying for each pixmap */
    if (!XGetWindowAttributes(dpy, win, &wa)) {
	fprintf(stderr, "What? Can't get attributes of window\n");
	abort();
    }
    if (wa.depth < 8) {
	/* Hopeless! Not enough colors...*/
	return E_NOBITMAP;
    }

    attr.valuemask = XpmCloseness | XpmExactColors | XpmColorKey | XpmColormap |
		    XpmDepth;
    attr.colormap = wa.colormap;
    attr.depth = wa.depth;
    attr.color_key = XPM_COLOR;
    attr.closeness = 10;
    attr.exactColors = _false_;
    sprintf(buf, template, bitpath, fname);
    if ((ret = XpmReadFileToPixmap(dpy, win, buf, pix, NULL, &attr)) ==
	 XpmSuccess) {
	
	if (attr.width > bit_width) bit_width = attr.width;
	if (attr.height > bit_height) bit_height = attr.height;
	full_pixmap[map] = _true_;
	return 0;
    }
    switch(ret) {
      case XpmColorError: return 0; /* partial success */
      case XpmSuccess: errmsg = "success"; break;
      case XpmOpenFailed: return E_NOBITMAP; /* open failed */
      case XpmFileInvalid: errmsg = "file format invalid"; break;
      case XpmNoMemory: errmsg = "No memory"; break;
      case XpmColorFailed: return E_NOCOLOR;
      default: errmsg = "unknown error code"; break;
    }
    fprintf(stderr, "XpmReadFileToPixmap (%s) failed: %s\n", buf, errmsg);
    return E_NOBITMAP;
}
#endif

/* Load in a single bitmap.  If this bitmap is the largest in the x or
 * y direction, set bit_width or bit_height appropriately.  If your pixmaps
 * are of varying sizes, a bit_width by bit_height box is guaranteed to be
 * able to surround all of them.
 * Return 0 for success, E_NOBITMAP if a loadable bitmap could not be found,
 * E_NOCOLOR if a loadable pixmap was found but there were not enough colors
 * load it.
 */

short LoadOneBitmap(char *fname, char *altname, Pixmap *pix, int map)
{
    if(bitpath && *bitpath) {
	/* we have something to try other than the default, let's do it */
#if USE_XPM
	switch(TryPixmapFile("%s/%s.xpm", pix, bitpath, fname, map)) {
	  case 0: return 0;
	  case E_NOCOLOR: return E_NOCOLOR;
	  case E_NOBITMAP: break;
	}
	switch(TryPixmapFile("%s/%s.xpm", pix, bitpath, altname, map)) {
	  case 0: return 0;
	  case E_NOCOLOR: return E_NOCOLOR;
	  case E_NOBITMAP: break;
	}
#endif
	if (TryBitmapFile("%s/%s.xbm", pix, bitpath, fname, map)) return 0;
	if (TryBitmapFile("%s/%s.xbm", pix, bitpath, altname, map)) return 0;
	return E_NOBITMAP;
    }

#if USE_XPM
      switch(TryPixmapFile("%s/%s.xpm", pix, BITPATH, fname, map)) {
	case 0: return 0;
	case E_NOCOLOR: return E_NOCOLOR;
	case E_NOBITMAP: break;
      }
      switch(TryPixmapFile("%s/%s.xpm", pix, BITPATH, altname, map)) {
	case 0: return 0;
	case E_NOCOLOR: return E_NOCOLOR;
	case E_NOBITMAP: break;
      }
#endif
    if (TryBitmapFile("%s/%s.xbm", pix, BITPATH, fname, map)) return 0;
    if (TryBitmapFile("%s/%s.xbm", pix, BITPATH, altname, map)) return 0;
    return E_NOBITMAP;
}

/* loads all the bitmaps in.. if any fail, it returns E_NOBITMAP up a level
 * so the program can report the error to the user.  It tries to load in the
 * alternates as well.
 */
short LoadBitmaps(void)
{
    register int i;
    short ret;

    if ((ret = LoadOneBitmap("man", NULL, &man, player))) return ret;
    if ((ret = LoadOneBitmap("saveman", "man", &saveman, playerstore)))
      return ret;
    if ((ret = LoadOneBitmap("object", NULL, &object, packet))) return ret;
    if ((ret = LoadOneBitmap("treasure", NULL, &treasure, save))) return ret;
    if ((ret = LoadOneBitmap("goal", NULL, &goal, store))) return ret;
    if ((ret = LoadOneBitmap("floor", NULL, &floor, ground))) return ret;

    if(optwalls) {
	for(i = 0; i < NUM_WALLS; i++) {
	    if ((ret = LoadOneBitmap(wallname[i], "wall", &walls[i], wall)))
	      return ret;
	}
    } else {
	if ((ret = LoadOneBitmap("wall", NULL, &walls[0], wall))) return ret;
    }
    return 0;
}

static void DrawPixmap(Drawable w, Pixmap p, int mapchar, int x, int y)
{
  if (full_pixmap[mapchar])
      XCopyArea(dpy, p, w, gc, 0, 0, bit_width, bit_height, x, y);
  else
      XCopyPlane(dpy, p, w, gc, 0, 0, bit_width, bit_height, x, y, 1);
}

/* Create and draw all the help windows.  This is not wholly foolproof with
 * the variable-size bitmap code yet, as the constants to place things on the
 * screen, are just that, constants.  This should be rewritten.
 */
void MakeHelpWindows(void)
{
  int i;
  int ypos = 0;
#if WWW
  char *title = "    Sokoban  --  X+WWW version 3.3c --  Help page %d";
#else
  char *title = "    Sokoban  --  X version 3.3c --  Help page %d";
#endif
  char *next = "     Press <Return> to exit";

  for(i = 0; i < HELP_PAGES; i++) {
    XFillRectangle(dpy, help[i], drgc, 0, 0, HELP_W, HELP_H);
    sprintf(buf, title, (i+1));
    XDrawImageString(dpy, help[i], gc, 0, 11, buf, strlen(buf));
    XDrawLine(dpy, help[i], gc, 0, 17, HELP_W, 17);
    XDrawLine(dpy, help[i], gc, 0, HELP_H-20, HELP_W, HELP_H-20);
    XDrawImageString(dpy, help[i], gc, 2, HELP_H-7, next, strlen(next));
  }
  for(i = 0; help_pages[i].textline != NULL; i++) {
    ypos += help_pages[i].ydelta;
    XDrawImageString(dpy,help[help_pages[i].page], gc,
                     help_pages[i].xpos * (finfo->max_bounds.width),
                     ypos, help_pages[i].textline,
                     strlen(help_pages[i].textline));
  }

  DrawPixmap(help[0], man, player, 180, 340);
  DrawPixmap(help[0], goal, store, 280, 340);
  DrawPixmap(help[0], walls[0], wall, 389, 340);
  DrawPixmap(help[0], object, packet, 507, 340);
  DrawPixmap(help[0], treasure, save, 270, 388);
  DrawPixmap(help[0], saveman, playerstore, 507, 388);
}

void ClearScreen(void)
{
  register int i,j;

  XFillRectangle(dpy, work, drgc, 0, 0, width, height);
  for(i = 0; i < MAXROW; i++)
    for(j = 0; j < MAXCOL; j++)
      DrawPixmap(work, floor, ground, j*bit_width, i*bit_height);
  XDrawLine(dpy, work, gc, 0, bit_height*MAXROW, bit_width*MAXCOL,
            bit_height*MAXROW);
}

void RedisplayScreen(void)
{
  if(hlpscrn == -1)
    XCopyArea(dpy, work, win, gc, 0, 0, width, height, 0, 0);
  else
    XCopyArea(dpy, help[hlpscrn], win, gc, 0, 0, HELP_W, HELP_H, 0, 0);
  XFlush(dpy);
}

void SyncScreen(void)
{
  XSync(dpy, 0);
}

void ShowScreen(void)
{
  register int i,j;

  for(i = 0; i < rows; i++)
    for(j = 0; j < cols && map[i][j] != 0; j++)
      MapChar(map[i][j], i, j, 0);
  DisplayLevel();
  DisplayPackets();
  DisplaySave();
  DisplayMoves();
  DisplayPushes();
  DisplayHelp();
  RedisplayScreen();
}

void MapChar(char c, int i, int j, Boolean copy_area)
{
  Pixmap this;

  this = GetObjectPixmap(i, j, c); /* i, j are passed so walls can be done */
  if (full_pixmap[(int)c])
  XCopyArea(dpy, this, work, gc, 0, 0, bit_width, bit_height, cX(j), cY(i));
  else
  XCopyPlane(dpy, this, work, gc, 0, 0, bit_width, bit_height, cX(j), cY(i), 1);
  if (copy_area) {
    XCopyArea(dpy, work, win, gc, cX(j), cY(i), bit_width, bit_height,
	      cX(j), cY(i));
  }
}

/* figures out the appropriate pixmap from the internal game representation.
 * Handles fancy walls.
 */
Pixmap GetObjectPixmap(int i, int j, char c)
{
  switch(c) {
    case player: return man;
    case playerstore: return saveman;
    case store: return goal;
    case save: return treasure;
    case packet: return object;
    case wall:
       if(optwalls) return walls[PickWall(i,j)];
       else return walls[0];
    case ground: return floor;
    default: return blank;
  }
}

/* returns and index into the fancy walls array. works by assigning a value
 * to each 'position'.. the type of fancy wall is computed based on how
 * many neighboring walls there are.
 */
int PickWall(int i, int j)
{
  int ret = 0;

  if(i > 0 && map[i-1][j] == wall) ret += 1;
  if(j < cols && map[i][j+1] == wall) ret += 2;
  if(i < rows && map[i+1][j] == wall) ret += 4;
  if(j > 0 && map[i][j-1] == wall) ret += 8;
  return ret;
}

/* Draws a string onto the working pixmap */
void DrawString(int x, int y, char *text)
{
  int x_off, y_off;

  x_off = x * finfo->max_bounds.width;
  y_off = y + finfo->ascent;

  XDrawImageString(dpy, work, gc, x_off, y_off, text, strlen(text));
}

/* The following routines display various 'statusline' stuff (ie moves, pushes,
 * etc) on the screen.  they are called as they are needed to be changed to
 * avoid unnecessary drawing */
void DisplayLevel(void)
{
   sprintf(buf, "Level: %3d", level);
   DrawString(0, STATUSLINE, buf);
}

void DisplayPackets(void)
{
   sprintf(buf, "Packets: %3d", packets);
   DrawString(12, STATUSLINE, buf);
}

void DisplaySave(void)
{
  sprintf(buf, "Saved: %3d", savepack);
  DrawString(26, STATUSLINE, buf);
}

void DisplayMoves(void)
{
  sprintf(buf, "Moves: %5d", moves);
  DrawString(38, STATUSLINE, buf);
}

void DisplayPushes(void)
{
  sprintf(buf, "Pushes: %3d", pushes);
  DrawString(52, STATUSLINE, buf);
}

void DisplayHelp(void)
{
  DrawString(0, HELPLINE, "Press ? for help.");
}

/* Function used by the help pager.  We ONLY want to flip pages if a key
 * key is pressed.. We want to exit the help pager if ENTER is pressed.
 * As above, <shift> and <control> and other such fun keys are NOT counted
 * as a keypress.
 */
Boolean WaitForEnter(void)
{
  KeySym keyhit;
  char buf[1];
  int bufs = 1;
  XComposeStatus compose;
  XEvent xev;

  while (1) {
    XNextEvent(dpy, &xev);
    switch(xev.type) {
      case Expose:
	RedisplayScreen();
	break;
      case KeyPress:
	buf[0] = '\0';
	XLookupString(&xev.xkey, buf, bufs, &keyhit, &compose);
	if(buf[0]) {
	  return (keyhit == XK_Return) ? _true_ : _false_;
	}
	break;
      default:
	break;
    }
  }
}

void ShowHelp(void)
{
  int i = 0;
  Boolean done = _false_;

  hlpscrn = 0;
  XCopyArea(dpy, help[i], win, gc, 0, 0, HELP_W, HELP_H, 0, 0);
  XFlush(dpy);
  while(!done) {
    done = WaitForEnter();
    if(done) {
      hlpscrn = -1;
      return;
    } else {
      i = (i+1)%HELP_PAGES;
      hlpscrn = i;
      XCopyArea(dpy, help[i], win, gc, 0, 0, HELP_W, HELP_H, 0, 0);
      XFlush(dpy);
    }
  }
}

short DisplayScores(short *newlev)
{
    return DisplayScores_(dpy, win, newlev);
}


void HelpMessage(void)
{
  XBell(dpy, 0);
  RedisplayScreen();
}
