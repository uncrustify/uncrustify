public class Class
{
    public void foo()
    {
        data.Sort(
            delegate(InputData lhs, InputData rhs)
            {
                return lhs.m_Name.CompareTo(rhs.m_Name);
            });
    }
}

// Want the braces aligning with the delegate keyword.

// Probably also an issue with lambda style delegates.
