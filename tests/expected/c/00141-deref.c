void foo(int *pA, int *pB)
{
   *pB = some.arr[*pA];

   foo(sizeof bar / sizeof *bar, baz);
}

#define MEM_READ_BYTE(phwi, addr, data) \
   *data = *((PUCHAR)((phwi)->m_pVirtualMemory + addr))

#define MEM_WRITE_BYTE(phwi, addr, data) \
   *((PUCHAR)((phwi)->m_pVirtualMemory + addr)) = (UCHAR)(data)

