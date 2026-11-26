// Test: a trailing comment must not be glued to the code when its line is
// aligned again by a group that started on a later line

void f(void)
{
	int a = 1;

	int           bb = 2; // c1
	unsigned long ccc = 3; // c2
	short         d = 4; /* c3 */

	std::string& e = name;
	double       ffffffffff = 6.0; /* c4 */
	long         g = 7; // c5
}

class Config
{
public:
int a;

int bb;     // c6
unsigned long ccc;     /* c7 */
};

void g(void)
{
	int                x   = 1; // c8
	unsigned long long yyyyyyyy = 2;
}
