/* the list of the options used to build the command line XrmDB.
 * if an option has no seperate arg, and appears on the command line, the 
 * last value in the table is what will be entered into the db.
 */
static XrmOptionDescRec options[] = {
  { "-w",          "*fancyWalls",    XrmoptionNoArg,    (caddr_t)  "on" },
  { "-walls",      "*fancyWalls",    XrmoptionNoArg,    (caddr_t)  "on" },
  { "-f",          "*fontName",      XrmoptionSepArg,   (caddr_t)     0 },
  { "-font",       "*fontName",      XrmoptionSepArg,   (caddr_t)     0 },
  { "-rv",         "*reverseVideo",  XrmoptionNoArg,    (caddr_t)  "on" },
  { "-reverse",    "*reverseVideo",  XrmoptionNoArg,    (caddr_t)  "on" },
  { "-fg",         "*foreground",    XrmoptionSepArg,   (caddr_t)     0 },
  { "-foreground", "*foreground",    XrmoptionSepArg,   (caddr_t)     0 },
  { "-bg",         "*background",    XrmoptionSepArg,   (caddr_t)     0 },
  { "-background", "*background",    XrmoptionSepArg,   (caddr_t)     0 },
  { "-bd",         "*borderColor",   XrmoptionSepArg,   (caddr_t)     0 },
  { "-border",     "*borderColor",   XrmoptionSepArg,   (caddr_t)     0 },
  { "-pr",         "*pointerColor",  XrmoptionSepArg,   (caddr_t)     0 },
  { "-pointer",    "*pointerColor",  XrmoptionSepArg,   (caddr_t)     0 },
  { "-b",          "*bitmapDir",     XrmoptionSepArg,   (caddr_t)     0 },
  { "-bitdir",     "*bitmapDir",     XrmoptionSepArg,   (caddr_t)     0 },
  { "-display",    ".display",	     XrmoptionSepArg,	(caddr_t)     0 },
  { "-xrm",        (char *) 0,       XrmoptionResArg,   (caddr_t)     0 }
};
