static void
__marvel_access_rtc(void *info)
{
	struct marvel_rtc_access_info *rtc_access = info;

	register unsigned long __r0 __asm__("$0");
	register unsigned long __r16 __asm__("$16") = rtc_access->function;
	register unsigned long __r17 __asm__("$17") = rtc_access->index;
	register unsigned long __r18 __asm__("$18") = rtc_access->data;
	
	__asm__ __volatile__(
		"call_pal %4 # cserve rtc"
		: "=r"(__r16), "=r"(__r17), "=r"(__r18), "=r"(__r0)
		: "i"(PAL_cserve), "0"(__r16), "1"(__r17), "2"(__r18)
		: "$1", "$22", "$23", "$24", "$25");

	rtc_access->data = __r0;
}

