#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

extern int atoi(char *);

#define TRY(name,expr) if (0>(expr)) { perror(name); exit(-1); }

char buf[4096];

int lingerval = 1;
 
main(int argc, char **argv)
{
    char *hostname;
    int port;
    int sock;
    int input_closed = 0;
    struct hostent *h;
    struct sockaddr_in client;
    if (argc != 3) {
	fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
	exit(-1);
    }
    hostname = argv[1];
    port = atoi(argv[2]);
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
	else bcopy(h->h_addr, &client.sin_addr, h->h_length);
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
    fprintf(stderr, "Connected.\n");
    for(;;) {
	fd_set rds;
	int n;
	FD_ZERO(&rds);
	if (!input_closed) FD_SET(0, &rds);
	FD_SET(sock, &rds);
	n = select(sock+1, &rds, 0, 0, 0);
	if (n>0) {
	    if (FD_ISSET(0, &rds)) {
		int ch = read(0, buf, sizeof(buf));
		if (ch == 0) input_closed = 1;
		write(sock, buf, ch);
	    }
	    if (FD_ISSET(sock, &rds)) {
		int ch = read(sock, buf, sizeof(buf));
		if (ch == 0) {
		    fprintf(stderr, "Connection closed by remote host\n");
		    exit(0);
		}
		write(1, buf, ch);
	    }
	}
    }
}
