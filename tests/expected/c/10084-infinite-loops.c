int main(void)
{
	int x;


	/* Braces on their own lines */

	while(1)
	{
	}

	while(1)
	{
	}

	while(1)
	{
	}

	while (1)
	{
	}

	while(1)
	{
	}


	/* Braces on the same lines as the keywords */

	while(1) {
	}

	while(1) {
	}

	while(1) {
	}

	while (1) {
	}

	while(1) {
	}


	/* Braces but everything on one line */

	while(1) {}

	while(1) {}

	while(1) {}

	while (1) {}

	while(1) {}


	/* No braces */

	while(1)
		x++;

	while(1)
		x++;

	while(1)
		x++;

	while (1)
		x++;

	while(1)
		x++;


	/* No braces and everything on one line */

	while(1) x++;

	while(1) x++;

	while(1) x++;

	while (1) x++;

	while(1) x++;


	/* Comments near or in loop syntax */

	for (; /*ever*/ ;)
		x++;

	while (/* always */ 1)
		x++;

	do
		x++;
	while (1) /* without end */;

	/* comment */ while(1)
		x++;

	while(1) // comment
		x++;

	while(1) // comment
		x++;
	// comment


	/* Unusual infinite loop conditions */

	for (; 999 ;)
		x++;

	while (999)
		x++;

	do
		x++;
	while (999);

	for (; 1 == 1 ;)
		x++;

	while (1 == 1)
		x++;

	do
		x++;
	while (1 == 1);
}
