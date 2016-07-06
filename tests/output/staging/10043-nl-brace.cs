public class Class : Base<int>
{
    public Class() : base(Value) {}
}
public class Foo
{
    public void foo()
    {
        // nl_brace_catch=force
        try { return 1; } catch (Exception) {}

        // nl_brace_finally=force
        try { bar(); } finally { barr(); }

        var v = foo(yolo,
            new List<Type>()
            {
                new Type { Value = prop; }
            });
    }
}
