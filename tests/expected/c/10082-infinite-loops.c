int main(void)
{
	int x;


	/* Braces on their own lines */

	while(true)
	{
	}

	while (true)
	{
	}

	while(true)
	{
	}

	while(true)
	{
	}

	while(true)
	{
	}


	/* Braces on the same lines as the keywords */

	while(true) {
	}

	while (true) {
	}

	while(true) {
	}

	while(true) {
	}

	while(true) {
	}


	/* Braces but everything on one line */

	while(true) {}

	while (true) {}

	while(true) {}

	while(true) {}

	while(true) {}


	/* No braces */

	while(true)
		x++;

	while (true)
		x++;

	while(true)
		x++;

	while(true)
		x++;

	while(true)
		x++;


	/* No braces and everything on one line */

	while(true) x++;

	while (true) x++;

	while(true) x++;

	while(true) x++;

	while(true) x++;


	/* Comments near or in loop syntax */

	for (; /*ever*/ ;)
		x++;

	while (/* always */ 1)
		x++;

	do
		x++;
	while (1) /* without end */;

	/* comment */ while(true)
		x++;

	while(true) // comment
		x++;

	while(true) // comment
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
