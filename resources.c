#include <stdio.h>
#include <string.h>

#include "externs.h"
#include "globals.h"

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
