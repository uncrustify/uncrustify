#include <stdio.h>

#define CMD_CHECK(expr) do { { expr; } CMDAssert(); } while (0)
#define INTERNAL(expr)  do { internalUse = 1; { expr; } internalUse = 0; } while (0)

int func(n) {
	CMD_CHECK(fflush(stdout););
	CMD_CHECK(fflush(stdout));
	CMD_CHECK(fflush(stdout) );
	CMD_CHECK( fflush(stdout) );
	CMD_CHECK( fflush(stdout));
	CMD_CHECK( fflush(stdout););
	CMD_CHECK( fflush(stdout); );
	CMD_CHECK( INTERNAL(fflush(stdout)) );
	CMD_CHECK( INTERNAL(fflush(stdout) ); );
}
