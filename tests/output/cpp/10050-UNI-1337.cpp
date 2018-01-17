// Runtime\Allocator\BucketAllocator.cpp

void foo()
{
    void* p1 = new(ptr) Block(bucketsSize);
    // becomes...
    void* p1 = new(ptr)Block(bucketsSize);
    // missing space after ')'
}
