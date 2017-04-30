#define concat0(a0,a1) a0 ??=??= a1 // trigraph ##
#define concat1(a0,a1) a0 %:%: a1   // digraph ##


#define STRINGIFY0(s) ??= s   // trigraph #
#define STRINGIFY1(s) %: s    // digraph #

#define msg0(x) printf("%c: %d\n", ??=@ x, x)  // trigraph #@
#define msg1(x) printf("%c: %d\n", %:@ x, x)   // digraph #@

// trigraph {
void x()
??<

	// trigraph []
	char a ??(??) = "a";
	// diigraph []
	char b <::> = "b";

	bool f, g, h;
	f = g = h = true;

	// trigraph ||
	f = g ??!??! h;
	// trigraph |=
	f ??!= g;
	// trigraph |
	f = g ??! h;
	// trigraph ^=
	f ??'= g;
	// trigraph ^
	f = g ??' h;

	// trigraph [, ]
	int m ??( 5 ??);
	// digraph [, ]
	int n <: 5 :>;

// trigraph }
	return;
??>

// digraph {, }
int y()
<%
	return 1;
%>