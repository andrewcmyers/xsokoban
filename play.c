#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "externs.h"
#include "globals.h"
#include "display.h"

extern int abs(int);

/* forward declarations */
static Boolean RunTo(int, int);
static void PushMan(int, int);
static void FindTarget(int, int, int);
static void MoveMan(int, int);
static void DoMove(short);
static short TestMove(KeySym);
static void MakeMove(KeySym);
static void MultiPushPacket(int, int);
static Boolean ApplyMoves(int moveseqlen, char *moveseq);

/* defining the types of move */
#define MOVE            1
#define PUSH            2
#define SAVE            3
#define UNSAVE          4
#define STOREMOVE       5
#define STOREPUSH       6

/* This value merely needs to be greater than the longest possible path length
 * making it the number of squares in the array seems a good bet. 
 */
#define BADMOVE MAXROW*MAXCOL

/* some simple checking macros to make sure certain moves are legal when 
 * trying to do mouse based movement.
 */
#define ISPLAYER(x,y) ((map[x][y] == player) || (map[x][y] == playerstore))
#define ISCLEAR(x,y) ((map[x][y] == ground) || (map[x][y] == store))
#define ISPACKET(x,y) ((map[x][y] == packet) || (map[x][y] == save))

/* Some macros useful in MultiPushPacket */
#define D_RIGHT    0
#define D_UP       1
#define D_LEFT     2
#define D_DOWN     3

#define UNVISITED (BADMOVE+1)

#define SETPACKET(x, y) map[x][y] = (map[x][y] == ground) ? packet : save
#define CLEARPACKET(x, y) map[x][y] = (map[x][y] == packet) ? ground : store
#define CAN_GO(x, y, d) \
  (ISCLEAR(x+Dx[d], y+Dy[d]) && PossibleToReach(x-Dx[d], y-Dy[d]))
#define OPDIR(d) ((d+2) % 4)

/* Variables used in MultiPushPacket. */
static int pmap[MAXROW+1][MAXCOL+1][4];
static char dmap[MAXROW+1][MAXCOL+1][4];

static int x1, y1;
static int Dx[4] = {0, -1,  0, 1};
static int Dy[4] = {1,  0, -1, 0};
static int moveKeys[4] = {XK_Right, XK_Up, XK_Left, XK_Down};



static XEvent xev;
static POS tpos1, tpos2, lastppos, lasttpos1, lasttpos2;
static char lppc, ltp1c, ltp2c;
static short action, lastaction;

static Boolean shift = _false_;
static Boolean cntrl = _false_;
static Boolean displaying = _true_;
static KeySym oldmove;
static int findmap[MAXROW+1][MAXCOL+1];

#define MAXDELTAS 4

/** For stack saves. **/
struct map_delta {
    int x,y;
    char oldchar, newchar;
};
struct move_r {
    short px, py;
    short moves, pushes, saved, ndeltas;
    struct map_delta deltas[MAXDELTAS];
};
static struct move_r move_stack[STACKDEPTH];
static int move_stack_sp; /* points to last saved move. If no saved move, -1 */
static Map prev_map;
static void RecordChange(void);
static void UndoChange(void);
static void InitMoveStack(void);

static int tempsave;
/* The move index at which a temporary save request was made */

extern short Play(void)
{
  short ret;
  int bufs = 1;
  char buf[1];
  KeySym sym;
  XComposeStatus compose;
  
  displaying = _true_;
  ClearScreen();
  ShowScreen();
  InitMoveStack();
  tempsave = moves;
  ret = 0;
  while (ret == 0) {
    XNextEvent(dpy, &xev);
    switch(xev.type) {
      case Expose:
	/* Redisplaying is pretty cheap, so I don't care too much about it */
	RedisplayScreen();
	break;
      case ButtonPress:
	switch(xev.xbutton.button) {
	  case Button1:
	    /* move the man to where the pointer is. */
	    MoveMan(xev.xbutton.x, xev.xbutton.y);
	    RecordChange();
	    break;
	  case Button2:
	    /* Push a ball */
	    PushMan(xev.xbutton.x, xev.xbutton.y);
	    RecordChange();
	    break;
	  case Button3:
	    /* undo last move */
	    UndoChange();
	    ShowScreen();
	    break;
	  default:
	    /* I'm sorry, you win the irritating beep for your efforts. */
	    HelpMessage();
	    break;
	}
	RedisplayScreen();
	break;
      case KeyPress:
	buf[0] = '\0';
	(void) XLookupString(&xev.xkey, buf, bufs, &sym, &compose);
	cntrl = (xev.xkey.state & ControlMask) ? _true_ : _false_;
	shift = (xev.xkey.state & ShiftMask) ? _true_ : _false_;
	switch(sym) {
	  case XK_q:
	    /* q is for quit */
	    if(!cntrl)
	      ret = E_ABORTLEVEL;
	    break;
	  case XK_S:
	    if (shift || cntrl) {
		ret = DisplayScores(0);
		RedisplayScreen();
	    }
	    break;
	  case XK_s:
	    /* save */
	    if(!cntrl && !shift) {
	      ret = SaveGame();
	      if(ret == 0)
		ret = E_SAVED;
	    }
	    break;
	  case XK_question:
	    /* help */
	    if(!cntrl) {
	      ShowHelp();
	      RedisplayScreen();
	    }
	    break;
	  case XK_r:
	    /* ^R refreshes the screen */
	    if(cntrl)
	      RedisplayScreen();
	    break;
	  case XK_U:
	    /* Do a full screen reset */
	    if(!cntrl) {
	      tempsave = moves = pushes = 0;
	      ret = ReadScreen();
	      InitMoveStack();
	      if(ret == 0) {
		ShowScreen();
	      }
	    }
	    break;
	  case XK_u:
	    if(!cntrl) {
		UndoChange();
		ShowScreen();
	    } else {
		Bool ok;
		moves = pushes = 0;
		ret = ReadScreen();
		InitMoveStack();
		ok = ApplyMoves(tempsave, move_history);
		RecordChange();
		assert(ok);
		ShowScreen();
		assert(tempsave == moves);
	    }
	    break;
	  case XK_c:
	    if (!cntrl) {
		if (moves < MOVE_HISTORY_SIZE) tempsave = moves;
		else tempsave = MOVE_HISTORY_SIZE - 1;
	    }
	    break;
	  case XK_k:
	  case XK_K:
	  case XK_Up:
	  case XK_j:
	  case XK_J:
	  case XK_Down:
	  case XK_l:
	  case XK_L:
	  case XK_Right:
	  case XK_h:
	  case XK_H:
	  case XK_Left:
	    /* Ordinary move keys */
	    MakeMove(sym);
	    RecordChange();
	    break;
	  default:
	    /* I ONLY want to beep if a key was pressed.  Contrary to what
	     * X11 believes, SHIFT and CONTROL are NOT keys
	     */
            if(buf[0]) {
	      HelpMessage();
	    }
	    break;
	}
	break;
      case ClientMessage:
	{
	    XClientMessageEvent *cm = (XClientMessageEvent *)&xev;
	    if (cm->message_type == wm_protocols &&
		cm->data.l[0] == wm_delete_window)
		  ret = E_ENDGAME;
	}
	break;
      default:
	break;
    }
    /* if we solved a level, set it up so we get some score! */
    if((ret == 0) && (packets == savepack)) {
      scorelevel = level;
      scoremoves = moves;
      scorepushes = pushes;
      break;
    }
  }
  return ret;
}

extern Boolean Verify(int moveseqlen, char *moveseq)
{
    InitMoveStack();
    tempsave = moves = pushes = 0;
    if (ApplyMoves(moveseqlen, moveseq) && savepack == packets) {
	scorelevel = level;
	scoremoves = moves;
	scorepushes = pushes;
	return _true_;
    } else {
	return _false_;
    }
}

/* Perform a user move based on the key in "sym". */
static void MakeMove(KeySym sym)
{
  do {
    action = TestMove(sym);
    if(action != 0) {
      lastaction = action;
      lastppos.x = ppos.x;
      lastppos.y = ppos.y;
      lppc = map[ppos.x][ppos.y];
      lasttpos1.x = tpos1.x;
      lasttpos1.y = tpos1.y;
      ltp1c = map[tpos1.x][tpos1.y];
      lasttpos2.x = tpos2.x;
      lasttpos2.y = tpos2.y;
      ltp2c = map[tpos2.x][tpos2.y];
      DoMove(lastaction);
      /* store the current keysym so we can do the repeat. */
      oldmove = sym;
    }
  } while ((action != 0) && (packets != savepack) && (shift || cntrl));
}

/* make sure a move is valid and if it is, return type of move */
static short TestMove(KeySym action)
{
  short ret;
  char tc;
  Boolean stop_at_object;

  stop_at_object = cntrl;

  if((action == XK_Up) || (action == XK_k) || (action == XK_K)) {
    tpos1.x = ppos.x-1;
    tpos2.x = ppos.x-2;
    tpos1.y = tpos2.y = ppos.y;
    if (moves < MOVE_HISTORY_SIZE) move_history[moves] = 'k';
  }
  if((action == XK_Down) || (action == XK_j) || (action == XK_J)) {
    tpos1.x = ppos.x+1;
    tpos2.x = ppos.x+2;
    tpos1.y = tpos2.y = ppos.y;
    if (moves < MOVE_HISTORY_SIZE) move_history[moves] = 'j';
  }
  if((action == XK_Left) || (action == XK_h) || (action == XK_H)) {
    tpos1.y = ppos.y-1;
    tpos2.y = ppos.y-2;
    tpos1.x = tpos2.x = ppos.x;
    if (moves < MOVE_HISTORY_SIZE) move_history[moves] = 'h';
  }
  if((action == XK_Right) || (action == XK_l) || (action == XK_L)) {
    tpos1.y = ppos.y+1;
    tpos2.y = ppos.y+2;
    tpos1.x = tpos2.x = ppos.x;
    if (moves < MOVE_HISTORY_SIZE) move_history[moves] = 'l';
  }
  tc = map[tpos1.x][tpos1.y];
  if((tc == packet) || (tc == save)) {
    if(!stop_at_object) {
      if(map[tpos2.x][tpos2.y] == ground)
	ret = (tc == save) ? UNSAVE : PUSH;
      else if(map[tpos2.x][tpos2.y] == store)
	ret = (tc == save) ? STOREPUSH : SAVE;
      else
	ret = 0;
    } else
      ret = 0;
  } else if(tc == ground)
    ret = MOVE;
  else if(tc == store)
    ret = STOREMOVE;
  else
    ret = 0;
  return ret;
}

/* actually update the internal map with the results of the move */
static void DoMove(short moveaction)
{
  map[ppos.x][ppos.y] = (map[ppos.x][ppos.y] == player) ? ground : store;
  switch( moveaction) {
    case MOVE:
      map[tpos1.x][tpos1.y] = player;
      break;
    case STOREMOVE:
      map[tpos1.x][tpos1.y] = playerstore;
      break;
    case PUSH:
      map[tpos2.x][tpos2.y] = map[tpos1.x][tpos1.y];
      map[tpos1.x][tpos1.y] = player;
      pushes++;
      break;
    case UNSAVE:
      map[tpos2.x][tpos2.y] = packet;
      map[tpos1.x][tpos1.y] = playerstore;
      pushes++;
      savepack--;
      break;
    case SAVE:
      map[tpos2.x][tpos2.y] = save;
      map[tpos1.x][tpos1.y] = player;
      savepack++;
      pushes++;
      break;
    case STOREPUSH:
      map[tpos2.x][tpos2.y] = save;
      map[tpos1.x][tpos1.y] = playerstore;
      pushes++;
      break;
  }
  moves++;
  if (displaying) {
      DisplayMoves();
      DisplayPushes();
      DisplaySave();
      MapChar(map[ppos.x][ppos.y], ppos.x, ppos.y, 1);
      MapChar(map[tpos1.x][tpos1.y], tpos1.x, tpos1.y, 1);
      MapChar(map[tpos2.x][tpos2.y], tpos2.x, tpos2.y, 1);
      SyncScreen();
#ifdef HAS_USLEEP
      usleep(SLEEPLEN * 1000);
#endif
  }
  ppos.x = tpos1.x;
  ppos.y = tpos1.y;
}

/* find the shortest path to the target via a fill search algorithm */
static void FindTarget(int px, int py, int pathlen)
{
  if(!(ISCLEAR(px, py) || ISPLAYER(px, py)))
    return;
  if(findmap[px][py] <= pathlen)
    return;

  findmap[px][py] = pathlen++;

  if((px == ppos.x) && (py == ppos.y))
    return;

  FindTarget(px - 1, py, pathlen);
  FindTarget(px + 1, py, pathlen);
  FindTarget(px, py - 1, pathlen);
  FindTarget(px, py + 1, pathlen);
}

/* Do all the fun movement stuff with the mouse */
static void MoveMan(int mx, int my)
{
  int x, y;

  shift = cntrl = _false_;

  /* reverse the screen coords to get the internal coords (yes, I know this 
   * should be fixed) RSN */
  y = wX(mx);
  x = wY(my);

  /* make sure we are within the bounds of the array */
  if((x < 0) || (x > MAXROW) || (y < 0) || (y > MAXCOL)) {
    HelpMessage();
    return;
  }

  /* If the click was on a packet then try to 'drag' the packet */
  if(ISPACKET(x, y)) {
    MultiPushPacket(x, y);
    return;
  }

  /* if we clicked on the player or a wall (or an object but that was already
   * handled) the we don't want to move.
   */
  if(!ISCLEAR(x, y)) {
    HelpMessage();
    return;
  }
  if (!RunTo(x, y)) HelpMessage();
}

/* Return whether (x,y) is on the board */
static Boolean ValidPosn(int x, int y)
{
    return (x >= 0) && (x <= MAXROW) && (y >= 0) && (y <= MAXCOL);
}

/* 
   Find the object at a position orthogonal to (x, y) that is
   closest to (ppos.x, ppos.y), is separated from (x,y) only
   by empty spaces, and has the player or an empty space on the far
   side of (x,y), and is not directly opposite the destination space
   from the player; place its coordinates in (*ox, *oy) and return _true_.
   If no such object exists, return _false_.
*/
static Boolean FindOrthogonalObject(int x, int y, int *ox, int *oy)
{
    int dir;
    int bestdist = BADMOVE;
    Boolean foundOne = _false_;
    for (dir = 0; dir < 4; dir++) {
	int dx, dy, x1, y1, dist;
	switch(dir) {
	    default: dx = 1; dy = 0; break; /* use "default" to please gcc */
	    case 1: dx = -1; dy = 0; break;
	    case 2: dx = 0; dy = 1; break;
	    case 3: dx = 0; dy = -1; break;
	}
	if ((ppos.x == x && ((ppos.y - y)*dy) < 0)
	    || (ppos.y == y && ((ppos.x - x)*dx) < 0)) continue;
	/* Eliminate case where player would push in the opposite
	   direction. */
	x1 = x; y1 = y;
        while (ValidPosn(x1, y1)) {
	    x1 += dx; y1 += dy;
	    dist = abs(ppos.x - x1) + abs(ppos.y - y1);
	    if (dist <= bestdist && ISPACKET(x1, y1) &&
		(ISPLAYER(x1 + dx, y1 + dy) || ISCLEAR(x1 + dx, y1 + dy))) {
		if (dist < bestdist) {
		    bestdist = dist;
		    *ox = x1;
		    *oy = y1;
		    foundOne = _true_;
		    break;
		} else {
		    foundOne = _false_; /* found one that was just as good */
		}
	    }
	    if (!ISCLEAR(x1, y1)) break;
	}
    }
    return foundOne ? _true_ : _false_ ;
}


/* Kind of a self explanatory name, ehh? */
static Boolean PossibleToReach(int x, int y)
{
  int i,j;

  /* Fill the trace map */
  for(i = 0; i <= MAXROW; i++)
    for (j = 0; j <= MAXCOL; j++)
      findmap[i][j] = BADMOVE;

  /* flood fill search to find a shortest path to the push point. */
  FindTarget(x, y, 0);
  return (findmap[ppos.x][ppos.y] != BADMOVE);
}

/* Try to find the goal from (x, y), coming from 'direction', */
/* having walked 'dist' units.                                */
static int PushFromDir(int x, int y, int direction, int dist)
{
  int r, res, d, fx = ppos.x, fy = ppos.y;

  /* Have we been here before? */
  if (pmap[x][y][direction] != UNVISITED) {
    if (pmap[x][y][direction] == BADMOVE)
      return BADMOVE;
    else
      return dist+pmap[x][y][direction]-1;
  }

  /* Have we found the target? */
  if (x==x1 && y==y1) {
    pmap[x][y][direction] = 1;
    return dist;
  }

  /* Try going in all four directions. Choose the shortest path, if any */
  res = BADMOVE;
  pmap[x][y][direction] = BADMOVE;
  for (d = 0; d < 4; d++) {
    ppos.x = fx; ppos.y = fy;
    if (CAN_GO(x, y, d)) {
      CLEARPACKET(x, y);
      SETPACKET(x+Dx[d], y+Dy[d]);
      ppos.x = x;
      ppos.y = y;
      r = PushFromDir(x+Dx[d], y+Dy[d], OPDIR(d), dist+1);
      if (r < res) {
	dmap[x][y][direction] = d;
	res = r;
      }
      SETPACKET(x, y);
      CLEARPACKET(x+Dx[d], y+Dy[d]);
    }
  }
  pmap[x][y][direction] = (res == BADMOVE) ? BADMOVE : res-dist+1;
  ppos.x = fx;
  ppos.y = fy;
  return res;
}

/* 
   The player has pressed button two on a packet.
   Wait for the release of the button and then try to move the
   packet from the button press coord to the button release coord.
   Give help message (i.e. beep...) if it is not possible.
   Code by Jan Sparud.
*/
static void MultiPushPacket(int x0, int y0)
{
  int i, j, k, d = 0, r, result;
  char manChar; 

  /* Wait for button release */
  XNextEvent(dpy, &xev);
  while (!(xev.type == ButtonRelease &&
      (xev.xbutton.button == Button2 ||
       xev.xbutton.button == Button1)))
    XNextEvent(dpy, &xev);

  y1 = wX(xev.xbutton.x);
  x1 = wY(xev.xbutton.y);

  if (x0 == x1 && y0 == y1) {
	/* Player just clicked on a stone. If man is next to stone,
	   just push it once.
        */
    if(ISPLAYER(x0 - 1, y0))
      MakeMove(XK_Down);
    else if(ISPLAYER(x0 + 1, y0))
      MakeMove(XK_Up);
    else if(ISPLAYER(x0, y0 - 1))
      MakeMove(XK_Right);
    else if(ISPLAYER(x0, y0 + 1))
      MakeMove(XK_Left);
    else {
      /* we weren't right next to the object */
      HelpMessage();
      return;
    }
    return;
  }

  if (!ValidPosn(x1, y1) || (!ISCLEAR(x1, y1) && !ISPLAYER(x1, y1))) {
    HelpMessage();
    return;
  }

  /* Remove (temporarily) the player from the board */
  manChar = map[ppos.x][ppos.y];
  map[ppos.x][ppos.y] = map[ppos.x][ppos.y] == player ? ground : store; 

  /* Prepare the distance map */
  for (i=0; i<MAXROW; i++)
    for (j=0; j<MAXCOL; j++)
      if (ISCLEAR(i, j) || (i==x0 && j==y0))
	for (k=0; k<4; k++)
	  pmap[i][j][k] = UNVISITED;
      else
	for (k=0; k<4; k++)
	  pmap[i][j][k] = BADMOVE;

  /* Try to go from the start position in all four directions */
  result = BADMOVE;
  for (k = 0; k < 4; k++)
    if (CAN_GO(x0, y0, k)) {
      r = PushFromDir(x0, y0, OPDIR(k), 1);
      if (r < result) {
	d = OPDIR(k);
	result = r;
      }
    }

  /* Put the player on the board again */
  map[ppos.x][ppos.y] = manChar; 

  /* If we found a way (i.e. result < BADMOVE) then follow the route */
  if (result < BADMOVE) {
    for (i = x0, j = y0; !(i==x1 && j==y1);) {
      d = OPDIR(dmap[i][j][d]);
      RunTo(i+Dx[d], j+Dy[d]);
      i -= Dx[d];
      j -= Dy[d];
      MakeMove(moveKeys[OPDIR(d)]);
    }
  } else {
    /* Otherwise, beep */
    HelpMessage();
    return;
  }
}

/* Push a nearby stone to the position indicated by (mx, my). */
static void PushMan(int mx, int my)
{
  int i, x, y, ox, oy, dist;

  shift = cntrl = _false_;

  /* reverse the screen coords to get the internal coords (yes, I know this 
   * should be fixed) RSN */
  y = wX(mx);
  x = wY(my);

  /* make sure we are within the bounds of the array */
  if(!ValidPosn(x,y)) {
    HelpMessage();
    return;
  }

  /* If the click was on a packet then try to 'drag' the packet */
  if(ISPACKET(x, y)) {
    MultiPushPacket(x, y);
    return;
  }

  /* if we clicked on the player or a wall (or an object but that was already
   * handled) the we don't want to move.
   */
  if(!ISCLEAR(x, y)) {
    HelpMessage();
    return;
  }

  if (!FindOrthogonalObject(x, y, &ox, &oy)) {
    HelpMessage();
    return;
  }

  assert(x == ox || y == oy);
  dist = abs(ox - x) + abs(oy - y);

  if (x > ox) ox--;
  if (x < ox) ox++;
  if (y > oy) oy--;
  if (y < oy) oy++;

  /* (ox,oy) now denotes the place we need to run to to be able to push */

  if (ox != ppos.x || oy != ppos.y) {
      if (!ISCLEAR(ox, oy)) {
	HelpMessage();
	return;
      }
      if (!RunTo(ox, oy)) {
	HelpMessage();
	return;
      }
  }
  assert(ppos.x == ox && ppos.y == oy);

  for (i = 0; i < dist; i++) {
    if (ppos.x < x) MakeMove(XK_Down);
    if (ppos.x > x) MakeMove(XK_Up);
    if (ppos.y < y) MakeMove(XK_Right);
    if (ppos.y > y) MakeMove(XK_Left);
  }
}

/* Move the player to the position (x,y), if possible. Return _true_
   iff successful. The position (x,y) must be clear.
*/
static Boolean RunTo(int x, int y)
{
  int i,j,cx,cy;
  /* Fill the trace map */
  for(i = 0; i <= MAXROW; i++)
    for (j = 0; j <= MAXCOL; j++)
      findmap[i][j] = BADMOVE;
  /* flood fill search to find a shortest path to the push point. */
  FindTarget(x, y, 0);

  /* if we didn't make it back to the players position, there is no valid path
   * to that place.
   */
  if(findmap[ppos.x][ppos.y] == BADMOVE) {
    return _false_;
  } else {
    /* we made it back, so let's walk the path we just built up */
    cx = ppos.x;
    cy = ppos.y;
    while(findmap[cx][cy]) {
      if(findmap[cx - 1][cy] == (findmap[cx][cy] - 1)) {
	MakeMove(XK_Up);
	cx--;
      } else if(findmap[cx + 1][cy] == (findmap[cx][cy] - 1)) {
	MakeMove(XK_Down);
	cx++;
      } else if(findmap[cx][cy - 1] == (findmap[cx][cy] - 1)) {
	MakeMove(XK_Left);
	cy--;
      } else if(findmap[cx][cy + 1] == (findmap[cx][cy] - 1)) {
	MakeMove(XK_Right);
	cy++;
      } else {
	/* if we get here, something is SERIOUSLY wrong, so we should abort */
	abort();
      }
    }
  }
  return _true_;
}


static void InitMoveStack()
{
    move_stack_sp = -1;
    move_stack[0].moves = moves;
    move_stack[0].pushes = pushes;
    move_stack[0].saved = savepack;
    memcpy(prev_map, map, sizeof(map));
}

/* Add a record to the move stack that records the changes since the
   last map state (which is stored in "prev_map"). Update "prev_map"
   to contain the current map so the next call to "RecordChange()"
   will perform correctly.
   
   If the stack runs out of space, dump the oldest half of the
   saved moves and continue. Undoing past that point will jump
   back to the beginning of the level. If the user is using the
   mouse or any skill, should never happen.
*/
static void RecordChange()
{
    struct move_r *r = &move_stack[++move_stack_sp];
    int x,y, ndeltas = 0;
    assert(move_stack_sp < STACKDEPTH);
    if (move_stack_sp == STACKDEPTH - 1) {
	int shift = STACKDEPTH/2;
	memcpy(&move_stack[0], &move_stack[shift],
	      sizeof(struct move_r) * (STACKDEPTH - shift));
	move_stack_sp -= shift;
	r -= shift;
    }
    r[1].moves = moves;
    r[1].pushes = pushes;
    r[1].saved = savepack;
    r[1].px = ppos.x;
    r[1].py = ppos.y;
    for (x = 0; x <= MAXROW; x++) {
	for (y = 0; y <= MAXROW; y++) {
	    if (map[x][y] != prev_map[x][y]) {
		assert(ndeltas < MAXDELTAS);
		r->deltas[ndeltas].x = x;
		r->deltas[ndeltas].y = y;
		r->deltas[ndeltas].newchar = map[x][y];
		r->deltas[ndeltas].oldchar = prev_map[x][y];
		ndeltas++;
#if 0
		printf("Change (%d,%d) %c->%c\n", x, y, prev_map[x][y],
		       map[x][y]);
#endif
	    }
	}
    }
    r->ndeltas = ndeltas;
    if (ndeltas == 0) {
	move_stack_sp--; /* Why push an identical entry? */
    }
    memcpy(prev_map, map, sizeof(map));
}

static void UndoChange()
{
    if (move_stack_sp <= 0) {
	int ret;
	moves = pushes = 0;
	ret = ReadScreen();
	InitMoveStack();
	if (ret) {
	    fprintf(stderr, "Can't read screen file\n");
	    exit(-1);
	}
    } else {
	struct move_r *r = &move_stack[move_stack_sp];
	int i;
	moves = r->moves;
	pushes = r->pushes;
	savepack = r->saved;
	ppos.x = r->px;
	ppos.y = r->py;
	for (i = 0; i<r->ndeltas; i++) {
#if 0
	    printf("Applying reverse change: (%d,%d) %c->%c\n",
		r->deltas[i].x, r->deltas[i].y,
		   map[r->deltas[i].x][r->deltas[i].y], r->deltas[i].oldchar);
#endif
	    map[r->deltas[i].x][r->deltas[i].y] = r->deltas[i].oldchar;
	}
	move_stack_sp--;
	memcpy(prev_map, map, sizeof(map));
    }
}

char move_history[MOVE_HISTORY_SIZE];

/* ApplyMoves:

    Receive a move sequence, and apply it to the current level as if
    the player had made the moves, but without doing any screen updates.
    Return _true_ if the move sequence is well-formed; _false_ if not.

    "moveseqlen" must be the length in characters of "moveseq".

    A well-formed move string "moveseq" is a sequence of the characters
    "h,j,k,l" and "1-9".

	[hjkl]: move the man by one in the appropriate direction
	[HJKL]: move the man by two in the appropriate direction
	[1-9]: provide a count of how many times the next move
		should be executed, divided by two. Thus, "1" means
		repeat twice, "9" means repeat 18 times.  The next
		character must be one of [hjklHJKL].
*/
static Boolean SingleMove(char c)
{
    switch(c) {
	case 'h': MakeMove(XK_h); break;
	case 'H': MakeMove(XK_h); MakeMove(XK_h); break;
	case 'j': MakeMove(XK_j); break;
	case 'J': MakeMove(XK_j); MakeMove(XK_j); break;
	case 'k': MakeMove(XK_k); break;
	case 'K': MakeMove(XK_k); MakeMove(XK_k); break;
	case 'l': MakeMove(XK_l); break;
	case 'L': MakeMove(XK_l); MakeMove(XK_l); break;
	default: return _false_;
    }
    return _true_;
}

static Boolean ApplyMoves(int moveseqlen, char *moveseq)
{
    int i = 0;
    char lastc = 0;
    Bool olddisp = displaying;

    displaying = _false_;
    shift = _false_;
    cntrl = _false_;

    while (i < moveseqlen) {
	char c = moveseq[i++];
	if (lastc && c != lastc) RecordChange();
	lastc = c;
	switch (c) {
	    case 'h': case 'j': case 'k': case 'l':
	    case 'H': case 'J': case 'K': case 'L':
		SingleMove(c);
		break;
	    case '1': case '2': case '3':
	    case '4': case '5': case '6':
	    case '7': case '8': case '9':
		{
		    int reps = c - '0';
		    if (i == moveseqlen) {
			displaying = olddisp;
			return _false_;
		    }
		    c = moveseq[i++];
		    lastc = tolower(c);
		    while (reps--) {
			if (!SingleMove(c)) {
			    displaying = olddisp;
			    return _false_;
			}
		    }
		}
		break;
	}
    }
    RecordChange();
    displaying = olddisp;
    return _true_;
}

