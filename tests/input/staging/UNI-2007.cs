public class MyGenericClass<T> where T:IComparable { }

class MyClass<T, U>
    where T : class
    where U : struct
{ }

public class MyGenericClass<T> where T : IComparable, new()
{
    // The following line is not possible without new() constraint:
    T item = new T();
}

interface IMyInterface
{
}

class Dictionary<TKey, TVal>
    where TKey : IComparable, IEnumerable
    where TVal : IMyInterface
{
    public void Add(TKey key, TVal val)
    {
    }
}

class List<T>
{
    void Add<U>(List<U> items) where U: T { /*...*/ }
    void Add<U>(List<U> items) where U : T { /*...*/ }
}

extern T GetNodeFromGuid<T>(Guid guid) where T : INode;
extern T GetNodeFromGuid<T>(Guid guid) where T: INode;
