#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "externs.h"
#include "globals.h"
#include "defaults.h"
#include "display.h"

/* rewritten slightly from the xantfarm code by Jef Poskanzer */

/* get a resource from the specified db.  This helps support the -display
 * code, but could be used to get a resource that should only be specified
 * in a given place (ie either only command line, or only Xresource db)
 */
char *GetDatabaseResource(XrmDatabase db, char *res)
{
  char name[500];
  char class[500];
  char *type;
  XrmValue value;

  (void)sprintf(name, "%s.%s", progname, res);
  (void)sprintf(class, "%s.%s", CLASSNAME, res);

  if(XrmGetResource(db, name, class, &type, &value) == True)
    if(strcmp(type, "String") == 0)
      return (char *)value.addr;
  return (char *)0;
}

/* just calls the above routine with the general combined db */
char *GetResource(char *res)
{
  return GetDatabaseResource(rdb, res);
}

/* returns the color pixel for the given resource */
Boolean GetColorResource(char *res, unsigned long *cP)
{
  XColor color;
  char *rval = GetResource(res);

  if(rval == (char *)0)
    return _false_;
  if(XParseColor(dpy, cmap, rval, &color) != True)
    return _false_;
  if(XAllocColor(dpy, cmap, &color) != True)
    return _false_;
  *cP = color.pixel;
  return _true_;
}

/* Uses the global "cmap" to look up colors. Call exit() if no color
   can be found. XXX Yuck */
unsigned long GetColorOrDefault(Display *dpy,
				char *resource_name,
				int depth,
				char *default_name_8,
				Boolean default_white_2)
{
    XColor c;
    unsigned long pixel;
    if (GetColorResource(resource_name, &pixel)) {
	return pixel;
    } else {
	char *val;
	if (depth >= 8)
	  val = default_name_8;
	else
	  val = default_white_2 ? "white" : "black";
	if (XParseColor(dpy, cmap, val, &c) &&
	    XAllocColor(dpy, cmap, &c))
	  return c.pixel;
	if (XParseColor(dpy, cmap, default_white_2 ? "white" : "black", &c) &&
	    XAllocColor(dpy, cmap, &c))
	  return c.pixel;
	fprintf(stderr, "Cannot obtain color for %s\n", resource_name);
	exit(EXIT_FAILURE);
    }
}

XFontStruct *GetFontResource(char *font)
{
    char *rval = GetResource(font);
    XFontStruct *finfo;
    if (!rval) rval = DEF_FONT;
    finfo = XLoadQueryFont(dpy, rval);
    if (!finfo) {
	finfo = XLoadQueryFont(dpy, "fixed");
    }
    return finfo;
}

char *boolopts[] = {
  "true",
  "True",
  "on",
  "On",
  "yes",
  "Yes",
  "1"
};

/* convert a string to the 'boolean' type (I defined my own, thanks) */
Boolean StringToBoolean(char *str)
{
  int nboolopts = sizeof(boolopts)/sizeof(*boolopts), i;

  for(i = 0; i < nboolopts; i++)
    if(strcmp(str, boolopts[i]) == 0)
      return _true_;
  return _false_;
}
