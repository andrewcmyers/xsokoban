#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config_local.h"

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "externs.h"

#define TRY(name,expr) if (0>(expr)) { perror(name); return 0; }

char buf[4096];

int lingerval = 1;
 
/* Open a TCP-IP connection to machine "hostname" on port "port", and
   send it the text in "msg". Return all output from the connection
   in a newly-allocated string that must be freed to reclaim its
   storage.

   Return 0 on failure.
*/
char *qtelnet(char *hostname, int port, char *msg) {
    int sock;
    struct hostent *h;
    struct sockaddr_in client;
    char *retbuf = 0;
    int retlen = 0;
    int retpos = 0;
    TRY("socket", (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)));
    TRY("setsockopt[SO_REUSEADDR,1]",
	setsockopt(sock,
	   SOL_SOCKET,
	   SO_REUSEADDR,
	   &lingerval,
	   sizeof(lingerval)));
    
    if (!isdigit(hostname[0])) {
	h = gethostbyname(hostname);
	if (!h) { fprintf(stderr, "Unknown host: %s\n", hostname); exit(-1); }
	else memcpy(&client.sin_addr, h->h_addr, h->h_length);
    } else {
	int a,b,c,d;
	unsigned long hostaddr;
	sscanf(hostname, "%d.%d.%d.%d", &a, &b, &c, &d);
	hostaddr = (a<<24)|(b<<16)|(c<<8)|d;
	client.sin_addr.s_addr = htonl(hostaddr);
    }


    client.sin_family = AF_INET;
    client.sin_port = htons (port);

    TRY("connect", connect(sock, &client, sizeof(client)));
#if 0
    fprintf(stderr, "Connected to %s.\n", hostname);
#endif
    write(sock, msg, strlen(msg));

    retlen = 4096;
    retpos = 0;
    retbuf = (char *)malloc(retlen);

    for(;;) {
	fd_set rds;
	int n;
	FD_ZERO(&rds);
	FD_SET(sock, &rds);
	n = select(sock+1, &rds, 0, 0, 0);
	if (n>0) {
	    if (FD_ISSET(sock, &rds)) {
		int ch = read(sock, buf, sizeof(buf));
		if (ch == 0) {
#if 0
		    fprintf(stderr, "Connection closed by remote host\n");
#endif
		    break;
		}
		if (retpos + ch >= retlen) {
		    retlen *= 2;
		    retbuf = (char *)realloc(retbuf, retlen);
		}
		memcpy(retbuf + retpos, buf, ch);
		retpos += ch;
	    }
	}
    }
    (void)close(sock);
    retbuf[retpos] = 0;
    return retbuf;
}
