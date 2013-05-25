
void foo(void)
{
	int a = 0;
	while (a < 3)
		a++;

	while (b < a)
		b++;

	do
		a--;
	while (a > 0);

	for (a = 0; a < 10; a++)
		printf("a=%d\n", a);

	if (a == 10)
		printf("a looks good\n");

	if (state == ST_RUN)
		if ((foo < bar) &&
		    (bar > foo2))
			if (a < 5)
				a *= a;

	while (*ptr++ != ',')
	{
	}
}

// mod_full_brace_for = remove should not remove the braces in this example:
int main() {
	if(true)   // indent=1
		for(int i = 0; i < 3; i++) {
			if(false)
				continue; // indent=4
		}
	else
		return; // indent=2
}

// mod_full_brace_if = remove should not remove the braces in this example:
int main() {
	if(true) {
		for(int i = 0; i < 3; i++)
			if(false)
				continue; // indent=4
	}
	else
		return; // indent=2
}

int main()
{
	while (1) {
		if (0)
			break;
		switch (1) {
		case 1:
			break;
		}
	}
	return 0;
}
