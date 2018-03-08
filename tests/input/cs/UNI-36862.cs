public unsafe struct QueryKeyNameCommand : IInputDeviceCommandInfo
{
    public string ReadKeyName()
    {
        fixed(QueryKeyNameCommand* thisPtr = &this)
        {
            return StringHelpers.ReadStringFromBuffer(new IntPtr(thisPtr->nameBuffer), kMaxNameLength);
        }
    }
}
