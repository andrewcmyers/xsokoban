#ifndef _DISPLAY_H
#define _DISPLAY_H


extern short InitX(void);
/* 
   Initialize the X connection. Create and map the main window,
   initialize pixmaps and atoms.  Assumes that "dpy" has already been set by a
   call to "XOpenDisplay", and "display_alloc" is true. 
*/

extern void DestroyDisplay(void);
/* Destroy the X connection if it has been created */

extern void ClearScreen(void);
/* Clear the main window */

extern void RedisplayScreen(void);
/* 
   Redisplay the main window. Has to handle the help screens if one
   is currently active.
*/

extern void SyncScreen(void);
/* Flush all X events to the screen and wait for them to get there. */

extern void ShowScreen(void);
/*
    Draw all the neat little pictures and text onto the working pixmap
    so that RedisplayScreen is happy.
*/

extern void MapChar(char, int, int, Boolean);
/* 
   Draw a single pixmap, translating from the character map to the pixmap
   rendition. If "copy_area", also push the change through to the actual window.
*/

extern void DisplaySave(void);
extern void DisplayMoves(void);
extern void DisplayPushes(void);
/*
    Display these three attributes of the current game.
*/

extern short DisplayScores(short *);
/*
   Display scores. Return E_ENDGAME if user wanted to quit from here.
   If user middle-clicked on a level, and "newlev" is non-zero, put
   the level clicked-on into "newlev".
*/

extern void ShowHelp(void);
/* 
   Display the first help page, and flip help pages (one per key press)
   until a return is pressed.
*/

extern void HelpMessage(void);
/*
    Remind the user how to get help. Currently just beeps because the
    help message is always displayed.
*/

extern Boolean WaitForEnter(void);
/*
    Wait for the enter key to be pressed.
*/

extern Display *dpy;
extern Boolean display_alloc;

/* "dpy" is the X display. "display_alloc" is whether "dpy" is meaningful. */

extern Atom wm_delete_window, wm_protocols;
/* Some useful atoms, initialized by InitX */

extern XrmDatabase rdb;
/* The X resource database */
	
extern Colormap cmap;
extern Boolean ownColormap;



#endif /* _DISPLAY_H */
