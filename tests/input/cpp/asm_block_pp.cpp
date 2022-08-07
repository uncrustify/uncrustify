
void add128( uint64_t & rlo, uint64_t & rhi, uint64_t addlo ) {
#if defined(HAVE_X86_64_ASM)
  __asm__ ("addq %2, %0\n"
           "adcq $0, %1\n"
#if defined(__clang__)
             // clang cannot work properly with "g" and silently
             // produces hardly-workging code, if "g" is specified;
           : "+r" (rlo), "+r" (rhi)
           : "m" (addlo)
#else
           : "+g" (rlo), "+g" (rhi)
           : "g" (addlo)
#endif
    );
#else
    rlo += addlo;
    rhi += (rlo < addlo);
#endif
}
