public class A
{
	private synchronized static void g(){
		int x=1;
	}
	synchronized A f(){
		return null;
	}
	public void foo(){
		g();

		synchronized ( this )
		{
			g();
		}

		g();

		synchronized ( this )
		{
			synchronized ( this )
			{
				synchronized ( this )
				{
					g();
				}
			}

			g();
		}

		synchronized ( this )
		{
			g();
		}
	}
}
