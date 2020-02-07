@protocol SomeProtocol
Props Method(const Contents &options = {});
@end

@interface SomeClass
Props Method1(const Contents &options = {});
@end


@implementation SomeClass
Props Method1(const Contents options = {});
@end

void Method2(const Contents options = {}) {
}

void Method3(const Contents &options = { .text = 10 });
