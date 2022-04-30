Vector2? a;
Vector2 b;

void G()
{
	int? x = true ? null : (int?)2;
	var q = x == null ? y : z;
	var q2 = x == q ? y : z;
	var q3 = x == null ? (y = new Y()) : z;
	var q4 = x == q ? (y = new Y()) : z;

	var q5 = x == null ? y = new Y() : z;
	var q6 = x == q ? y = new Y() : z;
}
