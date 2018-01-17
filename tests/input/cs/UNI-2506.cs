public class Class
{
	public int property
	{
		get
		{
			return !IsModeActive(Mode.None)
				&& !IsModeActive(Mode.Foo)
				&& !IsModeActive(Mode.Bar);
		}
	}
}
