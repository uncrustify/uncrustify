#include <stdbool.h>
#include <stdio.h>

bool nested_boolean_example(int a, int b, int c, int d)
{
    // it looks a little bit weird when boolean operators are at the end of lines
	if ((a > 10 &&
	   (b < 5 || c > 7)) &&
	   (d != 0 &&
	   (a * d > b + c)))
	{
		return true;
	}
	return false;
}

bool complex_nested_conditions(int x, int y, int z)
{
	// it looks good when boolean operators are at the beginning of lines
	if ((x == y
	   || (y < z && z > 20))
	   && ((x + y > z * 2)
	   || (x - z < y * 3 && x > 100)))
	{
		return true;
	}
	return false;
}

int main()
{
	if ((type_ != type_a
	   || !use_type_a)
	   && (type_ != type_b
	   || !use_type_b)
	   && (type_ != type_c
	   || !use_type_c)
	   && (type_ != type_d
	   || !use_type_d)
	   && !use_something_else)
	{
		do_something();
	}
	int a = 15, b = 3, c = 8, d = 2;
	int x = 5, y = 5, z = 25;

	if (nested_boolean_example(a, b, c, d)
	   && complex_nested_conditions(x, y, z))
	{
		printf("Both nested conditions are true.\n");
	}

	// here also the != operator is indented more, because it is nested
        if (a < b
            && (some_ultra_long_name_function()
            != another_ultra_long_name_function()))
          {
	    printf("Complex condition with nested expressions is true.\n");
	  }

	if ((a < b
	   && (b == c
	   || d > 100))
	   || ((a * b - d < c * c + b)
	   && (d != 1)))
	{
		printf("Complex condition with nested expressions is true.\n");
	}

	return 0;
}
