int main(void)
{
	int x;


	/* Braces on their own lines */

	do
	{
	}while(1);

	do
	{
	}while(1);

	do
	{
	}
	while(1);

	do
	{
	}while(1);

	do
	{
	}
	while (1);


	/* Braces on the same lines as the keywords */

	do       {
	}while(1);

	do           {
	}while(1);

	do {
	} while(1);

	do        {
	}while(1);

	do {
	} while (1);


	/* Braces but everything on one line */

	do       {}while(1);

	do           {}while(1);

	do {} while(1);

	do        {}while(1);

	do {} while (1);


	/* No braces */

	do     {
		x++;
	}while(1);

	do         {
		x++;
	}while(1);

	do{
		x++;
	}
	while(1);

	do      {
		x++;
	}while(1);

	do{
		x++;
	}
	while (1);


	/* No braces and everything on one line */

	do     {x++;}while(1);

	do         {x++;}while(1);

	do{x++;}while(1);

	do      {x++;}while(1);

	do{x++;}while (1);


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
	}while(1);

	do    { // comment
		x++;
	}while(1);

	do{ // comment
		x++;
	}
	while (1); // comment


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
