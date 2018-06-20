//It deletes the space after {
class Foo 
{
    extern internal bool canAccess { [NativeMethod(Name = "CanAccessFromScript")] get; }

    extern public int subMeshCount { get; [NativeMethod(Name = "CanAccessFromScript")] set; } 

    [TestCase("tag1;tag2", new string[] {"tag1", "tag2"})]
    [TestCase("tag1 ; tag2", new string[] {"tag1", "tag2"})]
    [TestCase("tag1 ;", new string[] {"tag1"})]
    [TestCase("", new string[0])]
    [TestCase(";", new string[0])]
    public void SetFlags_iOS(string flags,  string[] expected)
    {
    }
}
