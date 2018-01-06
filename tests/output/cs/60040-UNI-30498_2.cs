class Foo
{
    public static IEnumerable<NPath> RuntimeIncludes { get; }
        = new[]
        {
        new NPath("Projects/PrecompiledHeaders")
        };

    void Foo(string file)
    {
        var type = Path.GetFileNameWithoutExtension(file);
        switch (Path.GetExtension(file))
        {
            case ".cs":
                resource = new Bar(string.Format("test output",
                    type));
                break;
            case ".baz":
                resource = new Baz(type,
                    string.Format(@"test output
with multiple
lines
",
                        type));
                break;
        }
    }
}

public class Bar
{
    private static FooBar Baz { get; }
        = new FooBar()
            .WithPath("foo/bar/baz")
            .WithSource("qux/quux/quuz")
            .WithPrebuiltReference(FooBar.Baz)
            .WithBaz("2")
            .Complete();
}
