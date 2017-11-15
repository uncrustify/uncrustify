//It deletes the space after {
class Foo
{
    extern internal bool canAccess { [NativeMethod(Name = "CanAccessFromScript")] get; }

    extern public int subMeshCount { get; [NativeMethod(Name = "CanAccessFromScript")] set; }
}
