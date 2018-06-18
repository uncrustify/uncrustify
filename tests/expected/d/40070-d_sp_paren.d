version(unittest) {
	import foo;
}

void main() {
	scope(exit) {
		foo();
	}

	scope(success) suckit();

	scope f = new Foo();
	if (foo)
	{
	}

	try {
		throw(e);
	}
	catch(Exception e) {
	}
}

