namespace Unity
{
    public class Class
    {
        // doesn't work because ; gets removed but
        public static readonly Class A = new Class() { name = "A", id = 1 };
        // works and ; doesn't get removed
        public static readonly Class B = new Class { name = "B", id = 2 };
    }
}
