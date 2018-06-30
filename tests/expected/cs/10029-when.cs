private static string GenerateHash()
{
    try
    {
        int i = 0;
    }
    catch when (DateTime.Now.DayOfWeek == DayOfWeek.Saturday)
    {
        int j = -1;
    }
    try
    {
        int i = 0;
    }
    catch (Exception e) when (DateTime.Now.DayOfWeek == DayOfWeek.Saturday)
    {
        var when = DateTime.Now;
        ulong kind = (ulong)(int)when.Kind;
        return ((kind << 62) | (ulong)when.Ticks).ToString();
    }
}
