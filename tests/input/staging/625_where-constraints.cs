interface D { } interface E { }

class C<T1, T2, TX, T3, T4, T5>
: IDisposable, IEnumerable<T1>
	where T1 : class,D ,E ,new()
		where T2 : IDictionary<D, Dictionary< string, float > >
where TX : struct, IDisposable
where T3: class
					where T4: D where T5:E
{
    void F<T3, T4, T5, TX, T6>()  where T3 : D, new()
    	where T4 : D
	where T5: D
	where TX : new()
	where T6: D
    {
    }

	class C2<T1, T2, TX, T3, T4, T5>
: IDisposable, IEnumerable<T1>
	where T1 : class,D ,E,new()
		where T2 : IDictionary<D, Dictionary< string, float > >
where TX : struct, IDisposable
	where T3: class
						where T4: D where T5:E
	{
	    void F2<T3, T4, T5, TX, T6>()
	    	where T3 : D, new()
	    	where T4 : D
		where T5: D
		where TX : new()
		where T6: D
	    {
	    }
	}
}
