void f(void (*g)())
{
}

int main()
{
	f([]() {
		int x = 1;
		switch (x) {
		case 1:
			break;
		}
	});
}
