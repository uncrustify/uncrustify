// Test: the space after a virtual brace must stay relative to the code
// before it when spaces were removed or added earlier on the line

void one(){if(bar()) baz(1);}
void two(){if(bar())   baz(2);}
void three(){while (*p++)   ;}
void four(){for (i = 0; i < n; i++) total  +=  i;}

void five(int a)
{
	int x  =  a; if (x) count++;
	int y  =  a; while (y--)   step(y);
}
