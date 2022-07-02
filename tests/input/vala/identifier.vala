namespace ns
{
// You can omit '@' in trivial 'type func-name' definitions.
void if() {
}
void else() {
}
void foreach() {
}

// Numeric identifiers require '@'.
int @123star() {
}

// Non-primitive return types require '@'.
Foo @while() {
}
}

public static int main()
{
	// You can omit '@' in trivial 'type var;' declarations.
	int for;
	int while = 1;
	int do;

	// Non-primitive types always require '@'.
	Foo @do;

	// Slightly more complex 'type var_list;' require '@'.
	int @if, @else, @for, @do, @while;

	// This will complain about missing '(' etc.
	// int if, else, for, do, while;

	// It is common to omit '@' when accessing methods.
	ns.if();
	ns.else();
	ns.do();
	ns.while();
	ns.foreach();

	// Numeric methods always require '@'.
	ns.@123star();

	return 0;
}
