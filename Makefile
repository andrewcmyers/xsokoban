# Generated automatically from Makefile.in by configure.
############################################################
# C compiler
############################################################
CC = gcc

############################################################
# C compiler flags
############################################################
#
OPTIMIZER = -O -DNDEBUG
#OPTIMIZER = -g -UNDEBUG
INCS = -I/usr/local/include -I/usr/local/include/X11 -I/usr/include/X11
WARN = -Wall
LIBS = -lXpm -lX11
DEFS =
OWNER = andru

############################################################
# Other programs
############################################################
INSTALL = /bin/installbsd -c -o $(OWNER)
CP = /bin/cp
CHOWN = /bin/chown
MKDIR = /bin/mkdir

############################################################
# Where to install the executable, data files, and man page
############################################################
INSTALL_BIN = /usr/local/bin
INSTALL_LIB = /usr/local/lib/xsokoban
INSTALL_MAN = /usr/local/man/man1

##### Nothing from here on should need customization ######################

CFLAGS = $(OPTIMIZER) $(WARN) $(INCS) $(DEFS)
OBJECTS = display.o main.o resources.o play.o score.o screen.o save.o \
	  scoredisp.o qtelnet.o

xsokoban: $(OBJECTS)
	$(CC) $(CFLAGS) -o xsokoban $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f xsokoban config.cache config.status Makefile

install: xsokoban
	$(INSTALL) -s -o $(OWNER) -m 4755 xsokoban $(INSTALL_BIN)/xsokoban
	$(INSTALL) xsokoban.man $(INSTALL_MAN)/xsokoban.1
	-$(MKDIR) $(INSTALL_LIB)
	-$(MKDIR) $(INSTALL_LIB)/scores
	-$(MKDIR) $(INSTALL_LIB)/saves
	-$(MKDIR) $(INSTALL_LIB)/screens
	$(CP) screens/screen.* $(INSTALL_LIB)/screens
	$(CP) -r bitmaps $(INSTALL_LIB)
	$(CHOWN) $(OWNER) $(INSTALL_LIB)/scores
	@echo "Remember to run 'xsokoban -c' if you have no score file yet."

# DO NOT DELETE THIS LINE -- make depend depends on it.

display.o: externs.h globals.h config.h defaults.h help.h display.h
main.o: externs.h config_local.h globals.h config.h options.h errors.h display.h
main.o: score.h www.h
play.o:  externs.h config_local.h globals.h config.h display.h
qtelnet.o: config_local.h externs.h  
resources.o:  externs.h config_local.h globals.h config.h defaults.h display.h
save.o: config_local.h externs.h globals.h config.h
score.o: config_local.h externs.h globals.h config.h score.h www.h
scoredisp.o: externs.h config_local.h globals.h config.h
scoredisp.o: defaults.h display.h score.h
screen.o: externs.h config_local.h globals.h config.h
