int main(void)
{
	int x;


	/* Braces on their own lines */

	for (;;)
	{
	}

	for(;;)
	{
	}

	for(;;)
	{
	}

	for(;;)
	{
	}

	for(;;)
	{
	}


	/* Braces on the same lines as the keywords */

	for (;;) {
	}

	for(;;) {
	}

	for(;;) {
	}

	for(;;) {
	}

	for(;;) {
	}


	/* Braces but everything on one line */

	for (;;) {}

	for(;;) {}

	for(;;) {}

	for(;;) {}

	for(;;) {}


	/* No braces */

	for (;;) {
		x++;
	}

	for(;;) {
		x++;
	}

	for(;;) {
		x++;
	}

	for(;;) {
		x++;
	}

	for(;;) {
		x++;
	}


	/* No braces and everything on one line */

	for (;;) {x++;}

	for(;;) {x++;}

	for(;;) {x++;}

	for(;;) {x++;}

	for(;;) {x++;}


	/* Comments near or in loop syntax */

	for (; /*ever*/ ;) {
		x++;
	}

	while (/* always */ 1)
		x++;

	do
		x++;
	while (1) /* without end */;

	/* comment */ for(;;) {
		x++;
	}

	for(;;) { // comment
		x++;
	}

	for(;;) { // comment
		x++;
	}
	// comment


	/* Unusual infinite loop conditions */

	for (; 999 ;) {
		x++;
	}

	while (999)
		x++;

	do
		x++;
	while (999);

	for (; 1 == 1 ;) {
		x++;
	}

	while (1 == 1)
		x++;

	do
		x++;
	while (1 == 1);
}
