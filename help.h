/* help pages */
typedef struct helpline {
   int xpos, ypos, page;
   char *textline;
} h_line;

h_line help_pages[] = {
  { 0, 35, 0, "Objective:  Push all the objects from their starting positions"},
  { 12, 50, 0, "into their goal positions.  Be warned that you can"},
  { 12, 65, 0, "push only one object at a time; watch out for corners!"},
  { 0, 100, 0, "Movement:   Use the vi-keys hjkl or the keyboard arrow keys"},
  { 25, 130, 0, "left right up down"},
  { 13, 145, 0, "Move/Push     h    l    k   j"},
  { 13, 160, 0, "Run/Push      H    L    K   J"},
  { 13, 175, 0, "Run Only     ^H   ^L   ^K  ^J"},
  { 0, 210, 0, "Commands:"},
  { 12, 210, 0, "^r: refresh screen            ?: get this help screen"},
  { 12, 225, 0, "^u: restore to temp save      c: make a temp save"}, 
  { 12, 240, 0, " u: undo last move/push       U: restart this level"},
  { 12, 255, 0, " s: save game and quit        q: quit game"},
  { 0, 285, 0, "Misc:       If you set a temp save, you need not undo all"},
  { 12, 300, 0, "when you get stuck.  Just reset to this save."},
  { 12, 330, 0, "A temporary save is automatically made at the start."},
  { 0, 380, 0, "Bitmaps:"},
  { 12, 380, 0, "Player:     Goal:      Wall:     Object:"},
  { 12, 420, 0, "Object on a goal:      Player on a goal:"},
  {0, 0, 0, NULL}
};

#define HELP_PAGES 1
