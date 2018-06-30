namespace Namespace
{
    public static class Class
    {
        public static void Foo()
        {
            Tests = Bar(
                A,
                cp =>
                    cp.Foo(new Bar
                    {
                        Identifier = "ID",
                        PathToEmbed = "VAL"
                    })
                        .WithPrebuiltReference(Moq),
                Core);

            var Test = FooBar(
                B,
                cp => cp.WithB(Bar).WithSource("Path/File.ext"),
                new[] {
                    A,
                    B,
                    C
                }
            );
        }
    }
}
