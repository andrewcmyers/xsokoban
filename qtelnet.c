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

int lingerval = 1;
 
/* see externs.h for spec */
extern char *qtelnet(char *hostname, int port, char *msg) {
    int sock;
    struct hostent *h;
    struct sockaddr_in client;
    char *retbuf = 0;
    int retmax = 0;
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

    TRY("connect", connect(sock, (struct sockaddr *)&client, sizeof(client)));
#if 0
    fprintf(stderr, "Connected to %s.\n", hostname);
#endif
    write(sock, msg, strlen(msg));

    retmax = 4096;
    retpos = 0;
    retbuf = (char *)malloc(retmax);

    for(;;) {
	int ch;
	if (retmax == retpos) {
	    retmax *= 2;
	    { char *nrb = (char *)malloc(retmax);
	      memcpy(nrb, retbuf, retmax);
	      free(retbuf);
	      retbuf = nrb; }
	}
	TRY("read", ch = read(sock, retbuf + retpos, retmax - retpos));
	if (ch == 0) {
#if 0
		fprintf(stderr, "Connection closed by remote host\n");
#endif
		break;
	}
	retpos += ch;
    }
    TRY("close", close(sock));
    retbuf[retpos] = 0;
    return retbuf;
}
