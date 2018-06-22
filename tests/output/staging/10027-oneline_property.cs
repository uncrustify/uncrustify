public class Class
{
    public float prop { get; set; }
    public float prop { get { return m; } }
    public float prop { set { m = value; } }
    public float prop { get { return m; } set { m = value; } }
    internal int prop { get { return m; } }
};
public class Container { public int prop { get; set; } };
