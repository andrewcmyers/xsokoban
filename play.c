#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "externs.h"
#include "globals.h"

extern int abs(int);

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

static XEvent xev;
static POS tpos1, tpos2, lastppos, lasttpos1, lasttpos2;
static char lppc, ltp1c, ltp2c;
static short action, lastaction;

static Boolean shift = _false_;
static Boolean cntrl = _false_;
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

/* play a particular level.
 * All we do here is wait for input, and dispatch to appropriate routines
 * to deal with it nicely.
 */
short Play(void)
{
  short ret;
  int bufs = 1;
  char buf[1];
  KeySym sym;
  XComposeStatus compose;
  
  ClearScreen();
  ShowScreen();
  InitMoveStack();
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
	      ret = E_ENDGAME;
	    break;
	  case XK_s:
	    /* save */
	    if(!cntrl) {
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
	      moves = pushes = 0;
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
	    /* A move, A move!! we have a MOVE!! */
	    MakeMove(sym);
	    RedisplayScreen();
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

/* Perform a user move based on the key in "sym". */
void MakeMove(KeySym sym)
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
short TestMove(KeySym action)
{
  short ret;
  char tc;
  Boolean stop_at_object;

  stop_at_object = cntrl;

  if((action == XK_Up) || (action == XK_k) || (action == XK_K)) {
    tpos1.x = ppos.x-1;
    tpos2.x = ppos.x-2;
    tpos1.y = tpos2.y = ppos.y;
  }
  if((action == XK_Down) || (action == XK_j) || (action == XK_J)) {
    tpos1.x = ppos.x+1;
    tpos2.x = ppos.x+2;
    tpos1.y = tpos2.y = ppos.y;
  }
  if((action == XK_Left) || (action == XK_h) || (action == XK_H)) {
    tpos1.y = ppos.y-1;
    tpos2.y = ppos.y-2;
    tpos1.x = tpos2.x = ppos.x;
  }
  if((action == XK_Right) || (action == XK_l) || (action == XK_L)) {
    tpos1.y = ppos.y+1;
    tpos2.y = ppos.y+2;
    tpos1.x = tpos2.x = ppos.x;
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
void DoMove(short moveaction)
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
  DisplayMoves();
  DisplayPushes();
  DisplaySave();
  MapChar(map[ppos.x][ppos.y], ppos.x, ppos.y, 1);
  MapChar(map[tpos1.x][tpos1.y], tpos1.x, tpos1.y, 1);
  MapChar(map[tpos2.x][tpos2.y], tpos2.x, tpos2.y, 1);
  ppos.x = tpos1.x;
  ppos.y = tpos1.y;
  SyncScreen();
}

/* undo the most recently done move */
void UndoMove(void)
{
  map[lastppos.x][lastppos.y] = lppc;
  map[lasttpos1.x][lasttpos1.y] = ltp1c;
  map[lasttpos2.x][lasttpos2.y] = ltp2c;
  ppos.x = lastppos.x;
  ppos.y = lastppos.y;
  switch( lastaction) {
    case MOVE:
      moves--;
      break;
    case STOREMOVE:
      moves--;
      break;
    case PUSH:
      moves--;
      pushes--;
      break;
    case UNSAVE:
      moves--;
      pushes--;
      savepack++;
      break;
    case SAVE:
      moves--;
      pushes--;
      savepack--;
      break;
    case STOREPUSH:
      moves--;
      pushes--;
      break;
  }
  DisplayMoves();
  DisplayPushes();
  DisplaySave();
  MapChar(map[ppos.x][ppos.y], ppos.x, ppos.y, 1);
  MapChar(map[lasttpos1.x][lasttpos1.y], lasttpos1.x, lasttpos1.y, 1);
  MapChar(map[lasttpos2.x][lasttpos2.y], lasttpos2.x, lasttpos2.y, 1);
  SyncScreen();
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

/* find the shortest path to the target via a fill search algorithm */
void FindTarget(int px, int py, int pathlen)
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
void MoveMan(int mx, int my)
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

  if(ISPACKET(x, y)) {
    HelpMessage();
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
Boolean ValidPosn(int x, int y)
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
Boolean FindOrthogonalObject(int x, int y, int *ox, int *oy)
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

/* Push a nearby stone to the position indicated by (mx, my). */
void PushMan(int mx, int my)
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

  /* if we are clicking on an object, and are right next to it, we want to
   * move the object.
   */
  if(ISPACKET(x, y)) {
    if(ISPLAYER(x - 1, y))
      MakeMove(XK_Down);
    else if(ISPLAYER(x + 1, y))
      MakeMove(XK_Up);
    else if(ISPLAYER(x, y - 1))
      MakeMove(XK_Right);
    else if(ISPLAYER(x, y + 1))
      MakeMove(XK_Left);
    else {
      /* we weren't right next to the object */
      HelpMessage();
      return;
    }
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
Boolean RunTo(int x, int y)
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
    move_stack[0].pushes = moves;
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
	InitMoveStack();
	ret = ReadScreen();
	moves = pushes = 0;
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
