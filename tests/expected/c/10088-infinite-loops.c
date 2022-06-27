int main(void)
{
	int x;


	/* Braces on their own lines */

	do
	{
	}while(true);

	do
	{
	}while(true);

	do
	{
	}
	while (true);

	do
	{
	}while(true);

	do
	{
	}
	while(true);


	/* Braces on the same lines as the keywords */

	do       {
	}while(true);

	do           {
	}while(true);

	do {
	} while (true);

	do        {
	}while(true);

	do {
	} while(true);


	/* Braces but everything on one line */

	do       {}while(true);

	do           {}while(true);

	do {} while (true);

	do        {}while(true);

	do {} while(true);


	/* No braces */

	do     {
		x++;
	}while(true);

	do         {
		x++;
	}while(true);

	do{
		x++;
	}
	while (true);

	do      {
		x++;
	}while(true);

	do{
		x++;
	}
	while(true);


	/* No braces and everything on one line */

	do     {x++;}while(true);

	do         {x++;}while(true);

	do{x++;}while (true);

	do      {x++;}while(true);

	do{x++;}while(true);


	/* Comments near or in loop syntax */

	for (; /*ever*/ ;)
		x++;

	while (/* always */ 1)
		x++;

	do{
		x++;
	}
	while (1) /* without end */;

	/* comment */ do    {
		x++;
	}while(true);

	do    { // comment
		x++;
	}while(true);

	do{ // comment
		x++;
	}
	while(true); // comment


	/* Unusual infinite loop conditions */

	for (; 999 ;)
		x++;

	while (999)
		x++;

	do{
		x++;
	}
	while (999);

	for (; 1 == 1 ;)
		x++;

	while (1 == 1)
		x++;

	do{
		x++;
	}
	while (1 == 1);
}
