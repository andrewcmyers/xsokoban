#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "externs.h"
#include "globals.h"
#include "defaults.h"
#include "display.h"
#include "score.h"

#define DEBUG_FETCH 0

/* local functions */
static void DrawPanel(XWindowAttributes *wa, Window panel);
static void DrawScores(XWindowAttributes *wa, Window win);
static void DrawThumb(Window thumb);
static void PositionThumb(Window thumb);
static char *InitDisplayScores(Display *dpy, Window win);
static short InitialPosition(int *vposn);
static void handleMotion(XEvent *xev, Boolean dragging, Boolean *scores_dirty);
static void ComputeRanks();
static void TrimPosn();

#if WWW
static short ValidateLines(int top, int bottom);
#endif

static Boolean initted = _false_;
static unsigned long sb_bg, panel_bg[3], border_color, panel_fg, 
  text_color, text_highlight, thumb_colors[3], separation_color;
static unsigned int sb_width, panel_height, border_width, bevel_width,
  text_indent;
static unsigned int thumb_height, thumb_width;
static unsigned int bevel_darkening;
static unsigned long white;
static XWindowAttributes wa;
static GC gc, scroll_gc;
static XFontStruct *finfo, *score_finfo;
static char *selected_user;

static int font_height;
static int vmax, vposn;
static int win_height;
static int thumb_range;
static int yclip;
    
static u_short rank[MAXSCOREENTRIES];

extern short DisplayScores_(Display *dpy, Window win, short *newlev)
{
    Status status;
    XEvent xev;
    Window scrollbar, panel, thumb;
    short ret = 0;
    Boolean dragging = _false_;
    Boolean scores_dirty = _false_;

    selected_user = username;

    if (!initted) {
	char *msg = InitDisplayScores(dpy, win);
	if (msg) {
	    fprintf(stderr, msg);
	    exit(EXIT_FAILURE);
	}
    }
    ComputeRanks();
    status = XGetWindowAttributes(dpy, win, &wa); assert(status);
    scrollbar = XCreateSimpleWindow(dpy, win,
				    wa.width - sb_width, 0,
				    sb_width, wa.height - panel_height,
				    border_width, border_color, sb_bg);
    panel = XCreateSimpleWindow(dpy, win,
				0, wa.height - panel_height,
				wa.width, panel_height,
				0, 0, panel_bg[0]);
    XSelectInput(dpy, scrollbar, ExposureMask | KeyPressMask
	    | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XSelectInput(dpy, panel, ExposureMask | KeyPressMask);
    XClearWindow(dpy, win);

    font_height = score_finfo->max_bounds.ascent +
      score_finfo->max_bounds.descent;
    win_height = wa.height - panel_height - yclip;
    thumb_range = wa.height - panel_height - thumb_height;
    if ((ret = InitialPosition(&vposn))) return ret;
    vmax = font_height * (scoreentries + 2) - win_height;
    yclip = font_height * 3/2;
    /* coerce to int to make sure we do this in signed arithmetic */
    TrimPosn();
    thumb = XCreateSimpleWindow(dpy, scrollbar,
				0, 0,
				thumb_width, thumb_height,
				border_width, border_color, thumb_colors[0]);
    PositionThumb(thumb);
    XSelectInput(dpy, thumb, ExposureMask);

    XMapRaised(dpy, scrollbar);
    XMapRaised(dpy, panel);
    XMapRaised(dpy, thumb);
    {
	XRectangle rectangles[1];
	scroll_gc = XCreateGC(dpy, win, 0, 0);
	XCopyGC(dpy, gc, GCForeground | GCBackground | GCFunction |
		GCFont | GCLineWidth, scroll_gc);
	rectangles[0].x = 0;
	rectangles[0].y = yclip;
	rectangles[0].width = wa.width - sb_width;
	rectangles[0].height = wa.height - panel_height - yclip;
	XSetClipRectangles(dpy, scroll_gc, 0, 0, &rectangles[0], 1, Unsorted);
	XSetFont(dpy, scroll_gc, score_finfo->fid);
    }
    

    DrawScores(&wa, win);
      
    for (;;) {
	if (scores_dirty) {
	    if (0 == XPending(dpy)) {
		scores_dirty = _false_;
		PositionThumb(thumb);
		XClearWindow(dpy, win);
		DrawScores(&wa, win);
		XSync(dpy, False); /* make sure we don't get ahead */
	    }
	}
	XNextEvent(dpy, &xev);
	switch(xev.type) {
	  default:
	    fprintf(stderr,
		"Warning: unexpected X event type %d seen\n", xev.type);
	    break;
	  case ClientMessage:
	    {
		XClientMessageEvent *cm = (XClientMessageEvent *)&xev;
		if (cm->message_type == wm_protocols &&
		    cm->data.l[0] == wm_delete_window) {
		      ret = E_ENDGAME;
		      goto done;
		    }
		    
	    }
	  case NoExpose:
	    break;
	  case Expose: {
		XExposeEvent *expose = &xev.xexpose;
		Window w = expose->window;
		if (expose->count != 0) break;
		if (w == win) {
		    DrawScores(&wa, win);
		} else
		if (w == scrollbar) {
		} else
		if (w == thumb) {
		    DrawThumb(thumb);
		}
	        if (w == panel) {
		    DrawPanel(&wa, panel);
		}
		XFlush(dpy);
	    }
	    
	    break;
	    case KeyPress: {
		char buf[1];
		int buflen = 1;
		KeySym sym;
		XComposeStatus compose;
		buf[0] = 0;
		(void)XLookupString(&xev.xkey, &buf[0], buflen,
					      &sym, &compose);
		assert(status);
		switch(sym) {
		  case XK_Return:
		    goto done;
		  case XK_q:
		    ret = E_ENDGAME;
		    goto done;
		  default:
		    if (buf[0]) XBell(dpy, 0);
		    break;
		}
		
	    }
	    break;
	  case ButtonPress:
	    if (xev.xbutton.window == scrollbar) {
		dragging = _true_;
		handleMotion(&xev, _true_, &scores_dirty);
	    }
	    if (xev.xbutton.window == win) {
		int i = (xev.xbutton.y - yclip +
			 vposn - font_height/2)/font_height;
		if (i < 0) i = 0;
		if (i >= scoreentries) i = scoreentries - 1;
		switch (xev.xbutton.button) {
		  case Button1:
		    {
			char *tmp = scoretable[i].user;
			if (0 != strcmp(tmp, selected_user)) {
			    selected_user = tmp;
			    scores_dirty = _true_;
			}
			break;
		    }
		  case Button2:
		    if (newlev) {
			*newlev = scoretable[i].lv;
			goto done;
		    } else {
			XBell(dpy, 0);
		    }
		    break;
		  default:
		    break;
	      }
	    }
	    break;
	  case ButtonRelease:
	    dragging = _false_;
	    break;
	  case MotionNotify:
	    handleMotion(&xev, dragging, &scores_dirty);
	    break;
	}
    }
  done:
    XDestroySubwindows(dpy, win);
    XFreeGC(dpy, scroll_gc);
    return ret;
}

static void DrawScores(XWindowAttributes *wa, Window win)
{
    int first_index = vposn/font_height;
    int last_index = (vposn + win_height - 1)/font_height;
    int i;
    char * header = "Rank                             User  Level   Moves  Pushes   Date";
    XSetForeground(dpy, gc, text_color);
    XDrawString(dpy, win, gc, text_indent, font_height, header,
		strlen(header));
    XDrawLine(dpy, win, gc, 0, yclip-1, wa->width - sb_width,
	      yclip-1);
    
#if WWW
    {
	int top = scoreentries - first_index;
	int bottom = scoreentries - last_index - 1;
	if (ValidateLines(top, bottom)) {
	    fprintf(stderr, "Oops! Bad lines from server: %d-%d\n",
		first_index, last_index);
	    exit(EXIT_FAILURE);
	}
	first_index = scoreentries - top;
	last_index = scoreentries - bottom - 1;
    }
#endif

    for (i = first_index; i <= last_index && i < scoreentries; i++) {
	char buf[500];
	int y = yclip + (i+1) * font_height - vposn;
	if (i < last_index &&
	    scoretable[i + 1].lv != scoretable[i].lv) {
	    XSetForeground(dpy, scroll_gc, separation_color);
	    XDrawLine(dpy, win, scroll_gc, 0, y + 2, wa->width, y + 2);
	}
	if (0 == strcmp(scoretable[i].user, selected_user)) {
	    XSetForeground(dpy, scroll_gc, text_highlight);
	} else {
	    XSetForeground(dpy, scroll_gc, text_color);
	}
	if (rank[i] <= MAXSOLNRANK)
	    sprintf(buf, "%4d", rank[i]);
	else
	    sprintf(buf, "    ");
	sprintf(buf + 4, " %32s %4d     %4d     %4d   %s",
		VALID_ENTRY(i) ? scoretable[i].user : "CACHE ERROR",
		scoretable[i].lv, scoretable[i].mv, scoretable[i].ps,
		DateToASCII((time_t)scoretable[i].date));
	XDrawString(dpy, win, scroll_gc, text_indent, y, buf, strlen(buf));
    }
}

static void DrawPanel(XWindowAttributes *wa, Window panel)
{
    char *msg = "Press <Return> to continue, \"q\" to quit the game.";
    XSetForeground(dpy, gc, panel_bg[2]);
    XFillRectangle(dpy, panel, gc,
		   0, 0, wa->width - bevel_width, bevel_width);
    XFillRectangle(dpy, panel, gc,
		   0, 0, bevel_width, panel_height - bevel_width);
    XSetForeground(dpy, gc, panel_bg[1]);
    {
	XPoint triangle[3];
	triangle[0].x = wa->width;
	triangle[0].y = bevel_width;
	triangle[1].x = wa->width;
	triangle[1].y = 0;
	triangle[2].x = wa->width - bevel_width;
	triangle[2].y = bevel_width;
	XFillPolygon(dpy, panel, gc,
		     &triangle[0], 3, Convex, CoordModeOrigin);
	triangle[0].x = bevel_width;
	triangle[0].y = panel_height - bevel_width;
	triangle[1].x = triangle[0].x;
	triangle[1].y = panel_height;
	triangle[2].x = 0;
	triangle[2].y = panel_height;
	XFillPolygon(dpy, panel, gc,
		     &triangle[0], 3, Convex, CoordModeOrigin);
    }
    XFillRectangle(dpy, panel, gc,
		   bevel_width, panel_height - bevel_width,
		   wa->width - bevel_width, bevel_width);
    XFillRectangle(dpy, panel, gc,
		   wa->width - bevel_width, bevel_width,
		   bevel_width, panel_height - bevel_width);
    XSetForeground(dpy, gc, white);
    XDrawLine(dpy, panel, gc, 0, 0, bevel_width,
	      bevel_width);
    XSetForeground(dpy, gc, border_color);
    XDrawLine(dpy, panel, gc, 0, 0,
	      wa->width, 0);
    XSetForeground(dpy, gc, text_color);
    XDrawString(dpy, panel, gc, text_indent, panel_height -
		2 * bevel_width, msg, strlen(msg));
}
    
static void DrawThumb(Window thumb)
{
    int wm1 = thumb_width - 1;
    XSetForeground(dpy, gc, thumb_colors[2]);
    XFillRectangle(dpy, thumb, gc,
		   0, 0, wm1 - bevel_width, bevel_width);
    XFillRectangle(dpy, thumb, gc,
		   0, 0, bevel_width, thumb_height - bevel_width);
    XSetForeground(dpy, gc, thumb_colors[1]);
    {
	XPoint triangle[3];
	triangle[0].x = wm1;
	triangle[0].y = bevel_width;
	triangle[1].x = wm1;
	triangle[1].y = 0;
	triangle[2].x = wm1 - bevel_width;
	triangle[2].y = bevel_width;
	XFillPolygon(dpy, thumb, gc,
		     &triangle[0], 3, Convex, CoordModeOrigin);
	triangle[0].x = bevel_width;
	triangle[0].y = thumb_height - bevel_width;
	triangle[1].x = triangle[0].x;
	triangle[1].y = thumb_height;
	triangle[2].x = 0;
	triangle[2].y = thumb_height;
	XFillPolygon(dpy, thumb, gc,
		     &triangle[0], 3, Convex, CoordModeOrigin);
    }
    XFillRectangle(dpy, thumb, gc,
		   bevel_width, thumb_height - bevel_width,
		   wm1 - bevel_width, bevel_width);
    XFillRectangle(dpy, thumb, gc,
		   wm1 - bevel_width, bevel_width,
		   bevel_width, thumb_height - bevel_width);
    XSetForeground(dpy, gc, white);
    XDrawLine(dpy, thumb, gc, 0, 0, bevel_width, bevel_width);
}
#if !defined(STRDUP_PROTO)
char *strdup(const char *s)
{
    int l = strlen(s);
    char *ret = (char *)malloc((size_t)(l + 1));
    strcpy(ret, s);
    return ret;
}
#endif

static unsigned int GetIntResource(char *resource_name, unsigned int def)
{
    char *ret;
    ret = GetResource(resource_name);
    if (!ret) return def;
    return atoi(ret);
}

static u_short darken(u_short x)
{
    return (u_short)((u_int)x * (0xFFFF - bevel_darkening)/0xFFFF);
}

static u_short lighten(u_short x)
{
    return x + bevel_darkening - (u_short)(bevel_darkening * (u_int)x/0xFFFF);
}

/* Return 0 on success, an error message on error.
   shades[0] contains the requested color, shades[1] contains
   a darker version of the same color, shades[2] contains a
   lighter version. For use in drawing Motif-oid beveled panels.
*/
static char *GetColorShades(Display *dpy, 
		     XWindowAttributes *wa,
		     char *resource_name,
		     char *default_name_8,
		     Boolean default_white_2,
		     unsigned long shades[])
{
    char *rval = GetResource(resource_name);
    char buf[500];
    XColor normal, light, dark;
    if (!rval) {
	if (wa->depth >= 8) rval = default_name_8;
	else rval = default_white_2 ? "white" : "black";
    }
    if (!XParseColor(dpy, wa->colormap, rval, &normal)) {
	sprintf(buf, "Cannot parse color name for resource %s: %s",
		resource_name, rval);
	return strdup(buf);
    }
    dark.red = darken(normal.red);
    dark.green = darken(normal.green);
    dark.blue = darken(normal.blue);
    light.red = lighten(normal.red);
    light.green = lighten(normal.green);
    light.blue = lighten(normal.blue);
    if (!XAllocColor(dpy, wa->colormap, &normal) ||
	!XAllocColor(dpy, wa->colormap, &light) ||
	!XAllocColor(dpy, wa->colormap, &dark)) {
	sprintf(buf, "Cannot allocate color shades for resource %s: %s",
		resource_name, rval);
	return strdup(buf);
    }
    shades[0] = normal.pixel;
    shades[1] = dark.pixel;
    shades[2] = light.pixel;
    return 0;
}


/* Return 0 on success, else return an error message. */
static char *InitDisplayScores(Display *dpy, Window win)
{
    Status status = XGetWindowAttributes(dpy, win, &wa);
    XGCValues gc_values;
    u_long value_mask;
    if (!status) return "Cannot get window attributes";
    bevel_darkening = GetIntResource("bevel.darkening", 16000);
    sb_bg = GetColorOrDefault(dpy, "scrollbar.background",
			      wa.depth, "gray", _true_);
    GetColorShades(dpy, &wa,
		   "panel.background", "beige", _true_,
		   panel_bg);
		   
    panel_fg = GetColorOrDefault(dpy, "panel.foreground",
				 wa.depth, "black", _true_);
    border_color = GetColorOrDefault(dpy, "border.color",
				     wa.depth, "black", _false_);
    text_color = GetColorOrDefault(dpy, "text.color",
				   wa.depth, "black", _false_);
    text_highlight = GetColorOrDefault(dpy, "text.highlight",
				       wa.depth, "red3", _false_);
				     
    text_indent = GetIntResource("text.indent", 3);
    white = GetColorOrDefault(dpy, "highlight.color",
			      wa.depth, "white", _true_);
    border_width = GetIntResource("border.width", 1);
    sb_width = GetIntResource("scrollbar.width", 25);
    panel_height = GetIntResource("panel.height", 25);
    bevel_width = GetIntResource("bevel.width", 3);
    thumb_height = GetIntResource("scrollbar.thumb.height", sb_width);
    thumb_width = GetIntResource("scrollbar.thumb.width", sb_width);
    separation_color = GetColorOrDefault(dpy, "sep.color", wa.depth,
					 "gray", _false_);
    GetColorShades(dpy, &wa, "scrollbar.thumb.color", "gray", _true_,
		   thumb_colors);
    finfo = GetFontResource("panel.font");
    score_finfo = GetFontResource("text.font");
    if (!score_finfo || !finfo) {
	return "Either cannot get font for panel or for text";
    }

    gc_values.function = GXcopy;
    gc_values.foreground = panel_fg;
    gc_values.background = panel_bg[0];
    gc_values.line_width = 1;
    gc_values.font = finfo->fid;
    value_mask = GCForeground | GCBackground | GCFunction | GCFont |
      GCLineWidth;
      
    gc = XCreateGC(dpy, win, value_mask, &gc_values);
    initted = _true_;
    return 0;
}


static void ComputeRanks()
{
    int i;
    for (i = 0; i < scoreentries; i++) {
	rank[i] = SolnRank(i, 0);
    }
}

/* Make "vposn" an allowable position. */
static void TrimPosn()
{
    if (vposn >= vmax)
      vposn = vmax - 1;
    if (vposn < 0) vposn = 0; /* must do this after prev stmt */
}

#if WWW
static void ComputeRanksLines(int line1, int line2)
{
    int i;
    for (i = scoreentries - line2; i < scoreentries - line1; i++) {
	if (VALID_ENTRY(i)) rank[i] = SolnRank(i, 0);
    }
}

static short ValidateLines(int top, int bottom)
{
    int i,j;
    int line1, line2;
    short ret;

    if (bottom < 0) {
	top -= bottom;
	bottom = 0;
    }
#if DEBUG_FETCH
    fprintf(stderr, "Validating: %d - %d\n", bottom, top);
#endif
    for (i = top; i > bottom; i--) {
	if (i < scoreentries && !VALID_ENTRY(scoreentries - 1 - i)) break;
    }
    for (j = bottom; j < i; j++) {
	if (j < scoreentries && !VALID_ENTRY(scoreentries - 1 - j)) break;
    }
#if DEBUG_FETCH
    fprintf(stderr, "Now validating: %d - %d\n", j, i);
#endif
    if (i <= j) return 0;
    line2 = i + 1;
    line1 = j;
    assert (line1 < line2);
#if DEBUG_FETCH
    fprintf(stderr, "Fetch request: %d - %d\n", line1, line2);
#endif
    ret = FetchScoreLines_WWW(&line1, &line2);
#if DEBUG_FETCH
    fprintf(stderr, "Actually fetched: %d - %d\n", line1, line2);
#endif
    ComputeRanksLines(line1, line2);
    switch (ret) {
	case 0:
	    return 0;
	case E_OUTOFDATE: /* try again! */
#if DEBUG_FETCH
	    fprintf(stderr, "Out of date, trying again...\n");
#endif
	    return ValidateLines(top, bottom);
	default: return ret;
    }
}
#endif

static short InitialPosition(int *vposn)
{
    int fc2;
#if WWW
    int fc1;
    {
    /* get the number of lines in the file */
	int line1 = 0, line2 = 0;
	short ret = FetchScoreLevel_WWW(&line1, &line2);
#if DEBUG_FETCH
    fprintf(stderr, "Actually fetched: %d - %d\n", line1, line2);
#endif
	ComputeRanksLines(line1, line2);
	if (ret == E_OUTOFDATE) ret = 0;
	if (ret) return ret;
    }
    
#endif
    fc2 = FindCurrent();

#if WWW
/* We may have empty entries that cause FindCurrent() to return the
   wrong answer. Therefore, repeatedly grab the region around
   FindCurrent() until we determine that we have a valid region that
   includes the current level and is large enough to fill the screen.
*/
    do {
	int lines = (win_height - 1)/font_height + 2;
	int bottom = fc2 + lines/2;
	int top = bottom - lines;
	short ret;

	top = scoreentries - top;
	bottom = scoreentries - bottom - 1;
#if DEBUG_FETCH
	fprintf(stderr, "Guess of bracketed area: %d - %d\n", bottom, top);
#endif
	if ((ret = ValidateLines(top, bottom))) return ret;
	fc1 = fc2;
	fc2 = FindCurrent();
    } while (fc1 != fc2);
#endif

    assert(fc2 != -1);
    *vposn = (int)(fc2 * font_height) - (int)(win_height/2);
    return 0;
}

static void PositionThumb(Window thumb)
{
    int x = (thumb_width - sb_width)/2 - 1;
    int y = (int)((float)vposn/vmax * (float)thumb_range);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (y > thumb_range) y = thumb_range;
    XMoveWindow(dpy, thumb, x - 1, y - 1);
    /* subtract 1 to fudge thumb into place */
}

static void handleMotion(XEvent *xev, Boolean dragging, Boolean *scores_dirty)
{
    int old_vposn = vposn;
    int y = xev->xbutton.y;
    if (dragging) {
	vposn = (float)(y - (int)thumb_height/2)/
	  thumb_range * vmax;
	TrimPosn();
	if (old_vposn != vposn) {
	    *scores_dirty = _true_;
	}
    }
    XFlush(dpy);
}

