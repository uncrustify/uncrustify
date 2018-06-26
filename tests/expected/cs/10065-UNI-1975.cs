// typeof(Dictionary<, >)

// is getting changed to

// typeof(Dictionary<, >)

// (space added after comma)

// Definitely not typical for C#. Needs special handling.

public class Class
{
    public void foo(Type type)
    {
        if (type == typeof(List<>))
        {
        }
        else if (type == typeof(Dictionary<,>))
        {
            var bar = typeof(Dictionary<,>).Bar();
        }
    }
}
