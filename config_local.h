/*
    This file contains only definitions required to make compilation
    occur successfully. These definitions control whether various
    include files or extern declarations are needed.

    If the answer to any of these questions is yes, the corresponding
    symbol should be defined.
*/
/* Is there a prototype for "getpass"? */
#define GETPASS_PROTO

/* Are "htons" and "ntohs" defined in <machine/endian.h>? */
#define NEED_ENDIAN

/* Are "htons" and "ntohs" defined in <net/nh.h>? */
#undef NEED_NH

/* Are "htons" and "ntohs" defined in <netinet/in.h>? */
#undef NEED_NETINET_IN_H

/* Are "htons" and "ntohs" defined in <sys/byteorder.h>? */
#undef NEED_BYTEORDER

/* Is there a prototype for "fprintf" in <stdio.h>? */
#define FPRINTF_PROTO

/* Is there a prototype for "fclose" in <stdio.h>? */
#define FCLOSE_PROTO

/* Is there a prototype for "time"? */
#undef TIME_PROTO

/* Is there a prototype for "mktemp"? */
#undef MKTEMP_PROTO

/* Is there a prototype for "perror" in <errno.h>? */
#define PERROR_PROTO

/* Is there a prototype for "rename" in <unistd.h>? */
#define RENAME_PROTO
