#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>

/* The boolean typedef */
typedef enum { _false_ = 0, _true_ = 1 } Boolean;

/* local system calls that need externs */
extern char *strrchr(char *, char);
extern char *getenv(char *);

/* stuff from display.c */
extern short LoadBitmaps(void);
extern void MakeHelpWindows(void);
extern void ClearScreen(void);
extern void RedisplayScreen(void);
extern void ShowScreen(void);
extern void MapChar(char, int, int, int);
extern Pixmap GetObjectPixmap(int, int, char);
extern int PickWall(int, int);
extern void DrawString(int, int, char *);
extern void ClearString(int, int, int);
extern void DisplayLevel(void);
extern void DisplayPackets(void);
extern void DisplaySave(void);
extern void DisplayMoves(void);
extern void DisplayPushes(void);
extern void DisplayHelp(void);
extern void ShowHelp(void);
extern void HelpMessage(void);
extern void DestroyDisplay(void);
extern short InitX(void);

/* stuff from main.c */
extern short CheckCommandLine(int *, char **);
extern void main(int, char **);
extern short GameLoop(void);
extern short GetGamePassword(void);
extern void Error(short);
extern void Usage(void);

/* stuff from resources.c */
extern char *GetDatabaseResource(XrmDatabase, char *);
extern char *GetResource(char *);
extern Boolean StringToBoolean(char *);
extern Boolean GetColorResource(char *, unsigned long *);

/* stuff from play.c */
extern short Play(void);
extern void MakeMove(KeySym);
extern short TestMove(KeySym);
extern void DoMove(short);
extern void UndoMove(void);
extern void TempSave(void);
extern void TempReset(void);
extern Boolean WaitForEnter(void);
extern void MoveMan(int, int);
extern void FindTarget(int, int, int);

/* stuff from score.c */
extern short OutputScore(void);
extern short MakeNewScore(void);
extern short GetUserLevel(short *);
extern short Score(void);
extern short ReadScore(void);
extern short MakeScore(void);
extern short FindUser(void);
extern short FindPos(void);
extern short WriteScore(void);
extern void ShowScore(void);
extern void CopyEntry(short, short);

/* stuff from screen.c */
extern short ReadScreen(void);

/* stuff from save.c */
extern short SaveGame(void);
extern short RestoreGame(void);
