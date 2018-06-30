
int foo() {
	if ( a ) {
		return 1;
	}
	else if ( b ) {
		return 2;
	}

	if ( a ) {
		return 3;
	}
	else if ( b ) {
		return 4;
	}
	else {
		a = 5;
		return 5;
	}

	if ( a ) {
		return 6;
	}
	else{
		return 7;
	}

	if ( a )
		return 8;

	if ( b ) {
		return 9;
	}

	if ( b ) {
		{ b = 5; }
		return 9;
	}
}
