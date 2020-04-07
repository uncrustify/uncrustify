//example file
public class A
{
public void A(string a)
{
	if (a == null)
	{
		return;
	}

	fixed(char* ptr = a)
	{
		 a = a + a;
	}
}
}
