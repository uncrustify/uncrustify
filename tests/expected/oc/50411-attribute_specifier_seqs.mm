int w1[1];
int w2 [[maybe_unused]] = 0;
int w3 [[foo(w1[0])]];                                                                                          // unknown attribute foo
int w4 [[foo((w1[0]))]];                                                                                        // unknown attribute foo
int w5 [[foo(w1[0] [[maybe_unused]])]];                                                         // unknown attribute foo
int w6 [[foo(w1[0] [[maybe_unused]]), [[deprecated]]]];                         // expected ] before [[deprecated
int w7 [[w1[0]]] = 0;                                                                                           // expected ] before [ in w1[
int w8 [[ [[maybe_unused]] ]];                                                                          // expected ] before [[maybe_unused
int w9 [ [ foo ] ] = 0;

@implementation Foo
- (void) message {
	Foo* foo = [[Foo alloc] init];
}
@end

Foo* foo = [[Foo alloc] init];

[[Foo sharedInstance] broadcast:[world hello]];
