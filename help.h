/* help pages */
typedef struct helpline {
   int xpos, ydelta, page;
   char *textline;
} h_line;

h_line help_pages[] = {
  { 0, 35, 0, "Objective:  Push all the objects from their starting positions"},
  { 12, 15, 0, "into their goal positions.  Be warned that you can"},
  { 12, 15, 0, "push only one object at a time; watch out for corners!"},
  { 0, 35, 0, "Movement:"},
  { 12, 15, 0, "Left mouse button: move player to this point"},
  { 12, 15, 0, "Middle mouse button: push object to this point"},
  { 12, 15, 0, "Right mouse button: undo last action"},
  { 12, 15, 0, "Also, left or middle button can drag objects"},
  { 6, 20, 0, "Or use the arrow keys or hljk, as in vi:"},
  { 13, 15, 0, "Move/Push     h    l    k   j"},
  { 13, 15, 0, "Run/Push      H    L    K   J"},
  { 13, 15, 0, "Run Only     ^H   ^L   ^K  ^J"},
  { 0, 30, 0, "Other Commands:"},
  { 12, 15, 0, "^r: redraw screen             ?: this help message"}, 
  { 12, 15, 0, " u: undo last action          U: restart this level"},
  { 12, 15, 0, " s: save game and quit        q: quit level"},
  { 12, 15, 0, " c: snapshot posn            ^U: restore snapshot"},
  { 12, 15, 0, " S: view score file"},
  { 12, 50, 0, "Player:       Goal:       Wall:     Object:"},
  { 12, 50, 0, "Object on a goal:         Player on a goal:"},
  {0, 0, 0, NULL}
};

#define HELP_PAGES 1
