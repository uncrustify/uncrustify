#define A                   -3
#define B            163
#define C     2

void foo()
{
	const std::string   & targetName1      = pEntry->getTargetName();
	const Point3d_t       currentPosition1 = pSatOrbit->GetPositionAtTime(jdNow);
}

void foo2()
{
	const std::string   **targetName2      = pEntry->getTargetName();
	const Point3d_t       currentPosition2 = pSatOrbit->GetPositionAtTime(jdNow);
}

void foo2()
{
	const std::string   **targetName3      = pEntry->getTargetName();
	const Point3d_t       currentPosition3 = pSatOrbit->GetPositionAtTime(jdNow);
}

typedef int  MY_INT;
typedef int *MY_INTP;
typedef int (*foo_t)(void *bar);
typedef int (*somefunc_t)(void *barstool);

