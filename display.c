#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "externs.h"
#include "globals.h"
#include "defaults.h"
#include "help.h"

/* mnemonic defines to help orient some of the text/line drawing, sizes */
#define HELPLINE ((bit_height * MAXROW) + 30)
#define STATUSLINE ((bit_height * MAXROW) + 5)
#define HELP_H (bit_height * MAXROW)
#define HELP_W (bit_width * MAXCOL)

/* useful globals */
Display *dpy;
Window win;
int scr;
GC gc, rgc, drgc;
XFontStruct *finfo;
unsigned int width, height, depth, bit_height, bit_width;
Boolean display_alloc = False, font_alloc = False, gc_alloc = False,
        pix_alloc = False;
Boolean optwalls;
Colormap cmap;
Cursor this_curs;
static Pixmap help[HELP_PAGES], floor;
static Pixmap blank, work, man, saveman, goal, object, treasure, walls[NUM_WALLS];
int hlpscrn = -1;
char buf[500];

extern char *progname;
extern char map[MAXROW+1][MAXCOL+1];
extern short rows, cols, level, moves, pushes, packets, savepack;
extern char *bitpath;

/* names of the fancy wall bitmap files.  If you define a set of fancy
 * wall bitmaps, they must use these names
 */
char *wallname[] = {
 "lonewall.xbm", "southwall.xbm", "westwall.xbm", "llcornerwall.xbm",
 "northwall.xbm", "vertiwall.xbm", "ulcornerwall.xbm", "west_twall.xbm",
 "eastwall.xbm", "lrcornerwall.xbm", "horizwall.xbm", "south_twall.xbm",
 "urcornerwall.xbm", "east_twall.xbm", "north_twall.xbm", "centerwall.xbm"
};

/* Do all the nasty stuff like makeing the windows, setting all the defaults
 * creating all the pixmaps, loading everything, and mapping the window.
 * this does NOT do the XOpenDisplay() so that the -display switch can be
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

  /* these are always needed */
  scr = DefaultScreen(dpy);
  cmap = DefaultColormap(dpy, scr);
  depth = DefaultDepth(dpy, scr);

  /* here is where we figure out the resources and set the defaults.
   * resources can be either on the command line, or in your .Xdefaults/
   * .Xresources files.  They are read in and parsed in main.c, but used
   * here.
   */
  rval = GetResource(FONT);
  if(rval == (char *)0)
    rval = DEF_FONT;
  finfo = XLoadQueryFont(dpy, rval);
  if(finfo == (XFontStruct *)0)
    return E_NOFONT;
  font_alloc = _true_;

  rval = GetResource(REVERSE);
  if(rval != (char *)0) {
    reverse = StringToBoolean(rval);
  }

  if(!GetColorResource(FOREG, &fore))
    fore = BlackPixel(dpy, scr);

  if(!GetColorResource(BACKG, &back))
    back = WhitePixel(dpy, scr);

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

  /* walls are funny.  if a alternate bitpath has been defined, assume
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
  wattr.event_mask = (KeyPressMask | ExposureMask | ButtonPressMask);
  wattr.cursor = this_curs;

  /* whee, create the window, we create it with NO size so that we
   * can load in the bitmaps, we later resize it correctly
   */
  win = XCreateWindow(dpy, RootWindow(dpy, scr), 0, 0, width, height, 4,
		      CopyFromParent, InputOutput, CopyFromParent,
                      (CWBackPixel | CWBorderPixel | CWBackingStore |
                       CWEventMask | CWCursor), &wattr);

  /* this will set the bit_width and bit_height as well as loading
   * in the pretty little bitmaps
   */
  if(LoadBitmaps() == E_NOBITMAP)
    return E_NOBITMAP;
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

  /* kill the font */
  if(font_alloc)
    XFreeFont(dpy, finfo);

  /* destroy everything allocted right around the gcs.  Help windows are
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
  }
  /* okay.. NOW we can destroy the main window and the display */
  if(display_alloc) {
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
  }
}

/* Load in a single bitmap.  If this bitmap is the largest in the x or
 * y direction, set bit_width or bit_height appropriately.  If your pixmaps
 * are of varying sizes, a bit_width by bit_height box is guaranteed to be
 * able to surround all of them.
 */
Boolean LoadOneBitmap(char *fname, char *altname, Pixmap *pix)
{
  unsigned int dum1, dum2;
  int dum3, dum4;
  Boolean load_fail = _false_;
  char buf[1024];

  if(bitpath && *bitpath) {
    /* we have something to try other than the default, let's do it */
    sprintf(buf, "%s/%s", bitpath, fname);
    if(XReadBitmapFile(dpy, win, buf, &dum1, &dum2, pix, &dum3, &dum4) !=
       BitmapSuccess) {
      if(altname && *altname) {
        fprintf(stderr, "%s: Cannot find '%s/%s', trying alternate.\n",
                progname, bitpath, fname);
        sprintf(buf, "%s/%s", bitpath, altname);
        if(XReadBitmapFile(dpy, win, buf, &dum1, &dum2, pix, &dum3, &dum4) !=
           BitmapSuccess) {
          load_fail = _true_;
          fprintf(stderr, "%s: Cannot find '%s/%s', trying defaults.\n",
                  progname, bitpath, altname);
        } else {
	  if(dum1 > bit_width) bit_width = dum1;
	  if(dum2 > bit_height) bit_height = dum2;
          return _true_;
        }
      } else {
        load_fail = _true_;
        fprintf(stderr, "%s: Cannot find '%s/%s', trying alternate.\n",
                progname, bitpath, fname);
      }
    } else {
      if(dum1 > bit_width) bit_width = dum1;
      if(dum2 > bit_height) bit_height = dum2;
      return _true_;
    }
  }
  assert(!bitpath || !*bitpath || load_fail);
  sprintf(buf, "%s/%s", BITPATH, fname);
  if(XReadBitmapFile(dpy, win, buf, &dum1, &dum2, pix, &dum3, &dum4) !=
     BitmapSuccess) {
      if(altname && *altname) {
	  fprintf(stderr, "%s: Cannot find '%s', trying alternate.\n",
		  progname, fname);
	  sprintf(buf, "%s/%s", BITPATH, fname);
	  if(XReadBitmapFile(dpy, win, buf, &dum1, &dum2, pix, &dum3, &dum4) !=
	     BitmapSuccess) {
	      fprintf(stderr, "%s: Cannot find '%s'!\n", progname, altname);
	      return _false_;
	  } else {
	      if(dum1 > bit_width) bit_width = dum1;
	      if(dum2 > bit_height) bit_height = dum2;
	      return _true_;
	  }
      } else {
	  fprintf(stderr, "%s: Cannot find '%s'!\n", progname, fname);
	  return _false_;
      }
  } else {
      if(dum1 > bit_width) bit_width = dum1;
      if(dum2 > bit_height) bit_height = dum2;
      return _true_;
  }
}

/* loads all the bitmaps in.. if any fail, it returns E_NOBITMAP up a level
 * so the program can report the error to the user.  It tries to load in the
 * alternates as well.
 */
short LoadBitmaps(void)
{
  register int i;

  if(!LoadOneBitmap("man.xbm", NULL, &man)) return E_NOBITMAP;
  if(!LoadOneBitmap("saveman.xbm", "man.xbm", &saveman)) return E_NOBITMAP;
  if(!LoadOneBitmap("object.xbm", NULL, &object)) return E_NOBITMAP;
  if(!LoadOneBitmap("treasure.xbm", NULL, &treasure)) return E_NOBITMAP;
  if(!LoadOneBitmap("goal.xbm", NULL, &goal)) return E_NOBITMAP;
  if(!LoadOneBitmap("floor.xbm", NULL, &floor)) return E_NOBITMAP;

  if(optwalls) {
    for(i = 0; i < NUM_WALLS; i++) {
      if(!LoadOneBitmap(wallname[i], "wall.xbm", &walls[i])) return E_NOBITMAP;
    }
  } else {
    if(!LoadOneBitmap("wall.xbm", NULL, &walls[0])) return E_NOBITMAP;
  }
  return 0;
}

/* create and draw all the help windows in.  This is not wholly fullproff with
 * the variable size bitmap code yet, as the constants to place things on the
 * screen, are just that, constants.  This could most likley be reworked.
 */
void MakeHelpWindows(void)
{
  register int i;
  char *title =
    "    Sokoban  --  X version by Joseph L. Traub  --  Help page %d";
  char *next =
     "     Press Enter to exit  --   Any other key for next page.";

  for(i = 0; i < HELP_PAGES; i++) {
    XFillRectangle(dpy, help[i], drgc, 0, 0, HELP_W, HELP_H);
    sprintf(buf, title, (i+1));
    XDrawImageString(dpy, help[i], gc, 0, 11, buf, strlen(buf));
    XDrawLine(dpy, help[i], gc, 0, 17, HELP_W, 17);
    XDrawLine(dpy, help[i], gc, 0, HELP_H-20, HELP_W, HELP_H-20);
    XDrawImageString(dpy, help[i], gc, 2, HELP_H-7, next, strlen(next));
  }
  for(i = 0; help_pages[i].textline != NULL; i++) {
    XDrawImageString(dpy,help[help_pages[i].page], gc,
                     help_pages[i].xpos * (finfo->max_bounds.width),
                     help_pages[i].ypos, help_pages[i].textline,
                     strlen(help_pages[i].textline));
  }
  XCopyPlane(dpy, man, help[0], gc, 0, 0, bit_width, bit_height, 180, 360, 1);
  XCopyPlane(dpy, goal, help[0], gc, 0, 0, bit_width, bit_height, 270, 360, 1);
  XCopyPlane(dpy, walls[0], help[0], gc, 0, 0, bit_width, bit_height,
	     369, 360, 1);
  XCopyPlane(dpy, object, help[0], gc, 0, 0, bit_width, bit_height,
	     477, 360, 1);
  XCopyPlane(dpy, treasure, help[0], gc, 0, 0, bit_width, bit_height,
	     270, 400, 1);
  XCopyPlane(dpy, saveman, help[0], gc, 0, 0, bit_width, bit_height,
	     477, 400, 1);
}

/* wipe out the entire contents of the screen */
void ClearScreen(void)
{
  register int i,j;

  XFillRectangle(dpy, work, drgc, 0, 0, width, height);
  for(i = 0; i < MAXROW; i++)
    for(j = 0; j < MAXCOL; j++)
      XCopyPlane(dpy, floor, work, gc, 0, 0, bit_width, bit_height,
                 j*bit_width, i*bit_height, 1);
  XDrawLine(dpy, work, gc, 0, bit_height*MAXROW, bit_width*MAXCOL,
            bit_height*MAXROW);
}

/* redisplay the current screen.. Has to handle the help screens if one
 * is currently active..  Copys the correct bitmaps onto the window.
 */
void RedisplayScreen(void)
{
  if(hlpscrn == -1)
    XCopyArea(dpy, work, win, gc, 0, 0, width, height, 0, 0);
  else
    XCopyArea(dpy, help[hlpscrn], win, gc, 0, 0, HELP_W, HELP_H, 0, 0);
  XFlush(dpy);
}

/* Flush all X events to the screen and wait for them to get there. */
void SyncScreen(void)
{
  XSync(dpy, 0);
}

/* Draws all the neat little pictures and text onto the working pixmap
 * so that RedisplayScreen is happy.
 */
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

/* Draws a single pixmap, translating from the character map to the pixmap
 * rendition. If "copy_area", also push the change through to the actual window.
 */
void MapChar(char c, int i, int j, Boolean copy_area)
{
  Pixmap this;

  this = GetObjectPixmap(i, j, c); /* i, j are passed so walls can be done */
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

/* Displays the first help page, and flips help pages (one per key press)
 * until a return is pressed.
 */
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

/* since the 'press ? for help' is ALWAYS displayed, just beep when there is
 * a problem.
 */
void HelpMessage(void)
{
  XBell(dpy, 0);
  RedisplayScreen();
}
