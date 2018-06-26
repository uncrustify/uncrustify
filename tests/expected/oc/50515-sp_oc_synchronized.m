- (void) unobserveAllKeyPaths {
	@synchronized (self) {
	}
}
#if 1
#define SYNCHRONIZED_BEGIN(x) @synchronized (x) {
#define SYNCHRONIZED_END }
#else
#define SYNCHRONIZED_BEGIN(x)
#define SYNCHRONIZED_END
#endif
