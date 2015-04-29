Vector2? a;
Vector2 b;

void F(Vector2? a) {
}
void F(Vector2 b) {
}

void G()
{
	int? x = true ? null : (int?)2;
	var q = x == null ? y : z;
	var q2 = x == q ? y : z;
	var q3 = x == null ? (y = new Y()) : z;
	var q4 = x == q ? (y = new Y()) : z;

	// this does not work currently. the nullable detection code can't tell the difference between the beginning
	// of a subexpression on the 'true' clause of a ternary and the end of a nullable var definition. for now
	// the workaround is to wrap the subexpression in parens.
	//var q5 = x == null ? y = new Y() : z;
	//var q6 = x == q ? y = new Y() : z;
}
