############################################################
# C compiler
############################################################
CC = gcc
#CC = cc

############################################################
# Definitions for CFLAGS (C compiler flags)
############################################################
#
OPTIMIZER = -O -DNDEBUG
#OPTIMIZER = -g -UNDEBUG
# Most C compilers
#CFLAGS = $(OPTIMIZER)
# OSF 1.3
#CFLAGS = -std1 $(OPTIMIZER)
# gcc
CFLAGS = -Wall $(OPTIMIZER)
# Uncomment this only if you are building under an AFS
# environment, and then only if you know exactly what this does.
#CFLAGS = -O -DVICE -I/usr/local/include -I/usr/local/include/res

############################################################
# Libraries
############################################################
# Normal
#LIBS = -lX11
# Uncomment this only if you are building under an AFS
# environment, and then only if you know exactly what this does.
#LIBS = -L/usr/local/lib -lX11 $(AUTHLIBS) -L/usr/local/lib/res -lresolv
# OSF 1.3
LIBS = -lX11 -ldnet_stub

############################################################
# Where to install the executable
############################################################
BIN = /usr/local/bin

##### Nothing from here on should need customization ######################

OBJECTS = display.o main.o resources.o play.o score.o screen.o save.o

xsokoban: $(OBJECTS)
	$(CC) $(CFLAGS) -o xsokoban $(OBJECTS) $(LIBS)

install: xsokoban
	install -s xsokoban $(BIN)/xsokoban

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f xsokoban

# DO NOT DELETE THIS LINE -- make depend depends on it.

display.o: externs.h globals.h config.h defaults.h help.h config_local.h
main.o: externs.h globals.h config.h options.h errors.h config_local.h
play.o: externs.h globals.h config.h config_local.h
resources.o: globals.h config.h config_local.h
save.o: externs.h globals.h config.h config_local.h
score.o: externs.h globals.h config.h config_local.h
screen.o: externs.h globals.h config.h config_local.h
