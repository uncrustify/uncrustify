auto getOperatorsMap() {
	return [
		"/"		: TokenType.Slash,
		"/="	: TokenType.SlashAssign,
		"."		: TokenType.Dot,
		".."	: TokenType.DoubleDot,
		"..."	: TokenType.TripleDot,
		"&"		: TokenType.Ampersand,
		"&="	: TokenType.AmpersandAssign,
		"&&"	: TokenType.DoubleAmpersand,
		"|"		: TokenType.Pipe,
		"|="	: TokenType.PipeAssign,
		"||"	: TokenType.DoublePipe,
		"-"		: TokenType.Minus,
		"-="	: TokenType.MinusAssign,
		"--"	: TokenType.DoubleMinus,
		"+"		: TokenType.Plus,
		"+="	: TokenType.PlusAssign,
		"++"	: TokenType.DoublePlus,
		"<"		: TokenType.Less,
		"<="	: TokenType.LessAssign,
		"<<"	: TokenType.DoubleLess
	];
}
