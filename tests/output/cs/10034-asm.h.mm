static inline void atomic_retain(volatile int *p)
{
#if defined(_MSC_VER)
    _InterlockedIncrement((LONG volatile*)p);
#else
    __asm__(
        "lock incl  %0\n\t"
        : "+m" (*p)
        :
        : "cc", "memory"
        );
#endif
}

static inline void atomic_thread_fence(memory_order_release_t)
{
    __asm__ __volatile__
    (
        ASM_LWSYNC : : : "memory"
    );
}
