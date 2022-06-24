int main(void)
{
	int x;


	/* Braces on their own lines */

	for (;;)
	{
	}

	while (true)
	{
	}

	do
	{
	}
	while (true);

	while (1)
	{
	}

	do
	{
	}
	while (1);


	/* Braces on the same lines as the keywords */

	for (;;) {
	}

	while (true) {
	}

	do {
	} while (true);

	while (1) {
	}

	do {
	} while (1);


	/* Braces but everything on one line */

	for (;;) {}

	while (true) {}

	do {} while (true);

	while (1) {}

	do {} while (1);


	/* No braces */

	for (;;)
		x++;

	while (true)
		x++;

	do
		x++;
	while (true);

	while (1)
		x++;

	do
		x++;
	while (1);


	/* No braces and everything on one line */

	for (;;) x++;

	while (true) x++;

	do x++; while (true);

	while (1) x++;

	do x++; while (1);


	/* Comments near or in loop syntax */

	for (; /*ever*/ ;)
		x++;

	while (/* always */ 1)
		x++;

	do
		x++;
	while (1) /* without end */;

	/* comment */ for(;;)
		x++;

	for(;;) // comment
		x++;

	do // comment
		x++;
	while (1); // comment


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
