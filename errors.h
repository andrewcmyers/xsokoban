/* textual error messages corresponding to the #defines in globals.h */
char *errmess[] = {
    "illegal error number",
    "cannot open screen file",
    "more than one player position in screen file",
    "illegal char in screen file",
    "no player position in screenfile",
    "too many rows in screen file",
    "too many columns in screenfile",
    "quit the game",
    "cannot get your username",
    "cannot open savefile",
    "error writing to savefile",
    "cannot stat savefile",
    "error reading savefile",
    "cannot restore, your savefile has been altered",
    "game saved",
    "too many entries in score table",
    "cannot open score file",
    "error reading scorefile",
    "error writing scorefile",
    "illegal command line syntax",
    "illegal password",
    "level number too large in command line",
    "only the game owner is allowed to make a new score table",
    "cannot find file to restore",
    "cannot find bitmap file",
    "cannot open display",
    "cannot load font",
    "cannot allocate string memory",
    "could not load requested color"
};

/* usage message */
#define USAGESTR \
 "usage: %s [-{c|r|<nn>|C} -s[level] -display <disp> -{w|walls} -{f|font} \n\
                 -{rv|reverse} -{b|bitdir} <path> -{fg|foreground} <color> \n\
                 -{bg|background} <color> -{bd|border} <color> \n\
                 -{pr|pointer} <color> -xrm <arg>]\n\n"

char *usages[] = {
  "\t-c                  : create a new score file (superuser only)\n",
  "\t-r                  : restore a saved game.\n",
  "\t-s [level]          : show high score table or a portion of it.\n",
  "\t-<nn>               : play level <nn> (<nn> must be greater than 0)\n",
  "\t-C                  : use own colormap for displaying\n",
  "\t-display <disp>     : run on display <disp>\n",
  "\t-w                  :\n",
  "\t-walls              : use fancy walls\n",
  "\t-rv                 :\n",
  "\t-reverse            : reverse the foreground and backgound colors\n",
  "\t-f <fn>             :\n",
  "\t-font <fn>          : use font <fn>\n",
  "\t-b <path>           :\n",
  "\t-bitdir <path>      : use bitmaps found in directory <path>\n",
  "\t-fg <color>         :\n",
  "\t-foregound <color>  : use <color> as foreground color\n",
  "\t-bg <color>         :\n",
  "\t-backgound <color>  : use <color> as background color\n",
  "\t-bd <color>         :\n",
  "\t-border <color>     : use <color> as border color\n",
  "\t-pr <color>         :\n",
  "\t-pointer <color>    : use <color> as the pointer foreground color\n",
  "\t-xrm <arg>          : specify that <arg> is an X resource.\n",
  NULL
};
