//TestCase-001
internal struct MyStruct<T>
    where T : struct, IPrepareFrameJob
{
}

//TestCase-002
class MyClass<T, U>
    where T : class
    where U : struct
{
}

//TestCase-003
interface IMyInterface
{
}

class Dictionary<TKey, TVal>
    where TKey : IComparable, IEnumerable
    where TVal : IMyInterface
{
    public void Add(TKey key, TVal val) {}
}

long DeviceCommand<TCommand>(int deviceId, ref TCommand command)
    where TCommand : struct, IInputDeviceCommandInfo;

public virtual long OnDeviceCommand<TCommand>(ref TCommand command)
    where TCommand : struct, IInputDeviceCommandInfo;

long DeviceCommand<TCommand>(int deviceId, ref TCommand command)
    where TCommand : struct, IInputDeviceCommandInfo

public virtual long OnDeviceCommand<TCommand>(ref TCommand command)
    where TCommand : struct, IInputDeviceCommandInfo
