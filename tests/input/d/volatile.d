void foo(void)
{
while (nextSegmentIndex >= 0)
{
    Segment seg = map.segments[nextSegmentIndex--];
    volatile if (seg.count)
        {
            currentTable = seg.table;
            for (int j = currentTable.length - 1; j >= 0; --j)
            {
                if ((nextEntry = currentTable[j]) !is null)
                {
                    nextTableIndex = j - 1;
                    return;
                }
            }
        }
}

if (e)
    volatile
    {
        oldValue = e.value;
        e.value  = newValue;
    }
return oldValue;
}
