/* config_local.h.  Generated automatically by configure.  */
/*
    This file contains only definitions required to make compilation
    occur successfully. These definitions control whether various
    include files or extern declarations are needed.

    If the answer to any of these questions is yes, the corresponding
    symbol should be defined.
*/
/* Is there a prototype for "getpass"? */
#define GETPASS_PROTO 1

/* Are "htons" and "ntohs" defined in <machine/endian.h>? */
#define NEED_ENDIAN 1

/* Are "htons" and "ntohs" defined in <net/nh.h>? */
/* #undef NEED_NH */

/* Are "htons" and "ntohs" defined in <netinet/in.h>? */
#define NEED_NETINET_IN 1

/* Are "htons" and "ntohs" defined in <sys/byteorder.h>? */
/* #undef NEED_BYTEORDER */

/* Is there a prototype for "fprintf" in <stdio.h>? */
#define FPRINTF_PROTO 1

/* Is there a prototype for "fclose" in <stdio.h>? */
#define FCLOSE_PROTO 1

/* Is there a prototype for "time"? */
#define TIME_PROTO 1

/* Is there a prototype for "mktemp"? */
#define MKTEMP_PROTO 1

/* Is there a prototype for "perror" in <errno.h>? */
#define PERROR_PROTO 1

/* Is there a prototype for "rename" in <unistd.h>? or (choke) in
    <stdio.h>? */
#define RENAME_PROTO 1

/* Does the system support "strdup" and have a prototype in <string.h>? */
#define STRDUP_PROTO 1

/* Does the system have "usleep"? */
#define HAS_USLEEP 1
#define USLEEP_PROTO 1

/* Is there a <limits.h>? (Otherwise, there had better be a <sys/limits.h>) */
#define HAVE_LIMITS_H 1
#define HAVE_SYS_LIMITS_H 1

/* Is there a <sys/select.h>? (AIX requires it for fd_sets) */
#define HAVE_SYS_SELECT_H 1

#define BZERO_PROTO 1

/* Does <time.h> have a localtime proto? */
#define LOCALTIME_PROTO 1

/* Is there a nl_langinfo() call? */
#define HAVE_NL_LANGINFO 1

/* Is there a working <sys/param.h> */
#define HAVE_SYS_PARAM_H 1
