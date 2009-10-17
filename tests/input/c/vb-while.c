unsigned long xdl_adler32(unsigned long adler, unsigned char const *buf,
                          unsigned int len)
{
   int k;
   unsigned long s1 = adler & 0xffff;
   unsigned long s2 = (adler >> 16) & 0xffff;

   if (!buf)
      return 1;

   while (len > 0)
   {
      k = len < NMAX ? len :NMAX;
      len -= k;
      while (k >= 16)
      {
         DO16(buf);
         buf += 16;
         k -= 16;
      }
      if (k != 0)
         do
         {
            s1 += *buf++;
            s2 += s1;
         } while (--k);
      s1 %= BASE;
      s2 %= BASE;
   }

   return(s2 << 16) | s1;
}

void f(){
while(1)
if(2)
3;
else
4;
}

