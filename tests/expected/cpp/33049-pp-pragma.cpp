#include <stdio.h>
int main(int argc, char** argv)
{
	#ifdef DEBUG
		#define FORMAT "argc=%d\n"
		std::printf(FORMAT,argc);
		#undef FORMAT
	#endif DEBUG
	#ifdef _OPENMP
		#pragma omp parallel
		{
			printf("Hello from thread!\n");
		}
	#endif

	#pragma CoverageScanner(cov-off)
	__pragma( CoverageScanner(cov-off) )
	_Pragma( CoverageScanner(cov-off) )

	return 0;
}
