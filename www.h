#ifndef _WWW_H
#define _WWW_H

/*
   WWWHOST: Host where WWW scores are stored
*/
#ifndef WWWHOST
#define WWWHOST "xsokoban.lcs.mit.edu"
#endif

/*
   WWWPORT: Port at WWWHOST
*/
#ifndef WWWPORT
#define WWWPORT 80
#endif

/*
   WWWSCOREPATH: Path to access in order to store scores. $L means the
   current level number, $M is the string of moves, $U is the user name,
   $N is the length of the string of moves.
*/
#ifndef WWWSCORECOMMAND
#define WWWSCORECOMMAND "POST /cgi-bin/xsokoban/solve?$L,$U HTTP/1.0\n" \
                        "Content-type: text/plain\n" \
                        "Content-length: $N\n" \
                        "\n" \
                        "$M\n"
#endif

#ifndef WWWREADSCORECMD
#define WWWREADSCORECMD "GET /cgi-bin/xsokoban/scores HTTP/1.0\n\n"
#endif

/*
   WWWSCREENPATH: Path to access in order to get screen files. $L
   means the requested level number. Not currently used.
*/
#ifndef WWWSCREENPATH
#define WWWSCREENPATH "GET /cgi-bin/xsokoban/screen?level=$L HTTP/1.0\n\n"
#endif

/*
   WWWGETLEVELPATH: Path to access in order to get a user's level. $U
   is the user name.
*/
#ifndef WWWGETLEVELPATH
#define WWWGETLEVELPATH "GET /cgi-bin/xsokoban/user-level?user=$U HTTP/1.0\n\n"
#endif

/*
   WWWGETLINESPATH: Path to access in order to get a section of the
   score file by line number. The first %d is substituted with "line1",
   the second with "line2".
*/
#ifndef WWWGETLINESPATH
#define WWWGETLINESPATH \
  "GET /cgi-bin/xsokoban/score-lines?user=$U,line1=%d,line2=%d HTTP/1.0\n\n"
#endif

/*
   WWWGETSCORELEVELPATH: Path to access in order to get a section of the
   score file by line number. The first %d is substituted with "line1",
   the second with "line2".
*/
#ifndef WWWGETSCORELEVELPATH
#define WWWGETSCORELEVELPATH \
  "GET /cgi-bin/xsokoban/score-level?user=$U,level=$L HTTP/1.0\n\n"
#endif


#endif /* _WWW_H */
