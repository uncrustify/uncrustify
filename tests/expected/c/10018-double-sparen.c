#include <stdio.h>

int main() {
	FILE *f;
	if ( (f = fopen("/dev/null", "r") ) )
		puts("file is open");
	return 0;
}
