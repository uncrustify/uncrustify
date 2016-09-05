public class Class
{
    public Foo GetFoo()
    {
        return new Foo
           {
               enabled = false,
               showDebug = false,
               middleGrey = 0.5f,
               min = -3f,
               max = 3f,
               speed = 1.5f
           };
    }


    public override Bar GetBar()
    {
        return new Bar()
        {
            m_Name = TestPropertyName
        };
    }

    //It appears uncrustify is adding double-indentation no matter what, to the initializer block.
    // Both of the above examples start out at a different level of indentation, and both get double-indented past original.
}
