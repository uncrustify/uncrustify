using System;
using Stream = System.IO.MemoryStream;

class ResourceUse
{
    async System.Threading.Tasks.Task Run()
    {
        using (var first = Open()) { Use(first); }
        using(var first = Open())
        using   (var second = Open())
        {
            Use(first, second);
        }
        using (Open()) Use();
        await using (var resource = OpenAsync()) { Use(resource); }
        using var scoped = Open();
    }
}
