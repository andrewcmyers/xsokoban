#include <stdio.h>

main()
{
    for (;;) {
	char c, d;
	do { c = getchar(); } while (c=='\n');
	if (c == EOF) break;
	do { d = getchar(); } while (d=='\n');
	if (d == EOF) break;
	putchar(c);
	putchar(d);
	putchar('\n');
    }
}
