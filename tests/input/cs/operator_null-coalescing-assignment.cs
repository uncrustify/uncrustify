public class Program
{
public static void Main()
{
	var thing = new int?();
	thing ??= new int?();
}
}
