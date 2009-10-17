/**
 * this is a really tough case - perhaps we shouldn't keep any #if crap
 * unless they all end with the same stack?
 */
void foo(void)
{
   int a;
#ifdef CONFIG_BLK_DEV_INITRD
   if (initrd_start)
      ROOT_DEV = Root_RAM0;
#elif defined(CONFIG_ROOT_NFS)
   ROOT_DEV = Root_NFS;
#elif defined(CONFIG_BLK_DEV_IDEDISK)
   ROOT_DEV = Root_HDA1;
#else
   ROOT_DEV = Root_SDA1;
#endif
   return;
}

