/* help pages */
typedef struct helpline {
   int xpos, ypos, page;
   char *textline;
} h_line;

h_line help_pages[] = {
  { 0, 35, 0, "Objective:  Push all the objects from their starting positions"},
  { 12, 50, 0, "into their goal positions.  Be warned that you can"},
  { 12, 65, 0, "push only one object at a time; watch out for corners!"},
  { 0, 100, 0, "Movement:"},
  { 12, 100, 0, "Left mouse button: move player to this point"},
  { 12, 115, 0, "Middle mouse button: push ball to this point"},
  { 12, 130, 0, "Right mouse button: undo last action"},
  { 0, 155, 0, "Or use the arrow keys or hljk, as in vi:"},
  { 13, 170, 0, "Move/Push     h    l    k   j"},
  { 13, 185, 0, "Run/Push      H    L    K   J"},
  { 13, 200, 0, "Run Only     ^H   ^L   ^K  ^J"},
  { 0, 225, 0, "Commands:"},
  { 12, 240, 0, "^r: redraw screen             ?: this help message"}, 
  { 12, 255, 0, " u: undo last action          U: restart this level"},
  { 12, 270, 0, " s: save game and quit        q: quit game"},
  { 0, 380, 0, "Bitmaps:"},
  { 12, 380, 0, "Player:     Goal:      Wall:     Object:"},
  { 12, 420, 0, "Object on a goal:      Player on a goal:"},
  {0, 0, 0, NULL}
};

#define HELP_PAGES 1
