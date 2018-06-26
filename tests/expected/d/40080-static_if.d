ClLinearExpression opBinary(string op) (double constant)
{
    static if (op == "+")
        return new ClLinearExpression(this, 1, constant);
    else static if (op == "-")
        return new ClLinearExpression(this, 1, -constant);
    else static if (op == "*")
        return new ClLinearExpression(this, constant, 0);
    else static if (op == "/")
        return new ClLinearExpression(this, 1.0 / constant, 0);
}
