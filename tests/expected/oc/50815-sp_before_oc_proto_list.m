@protocol ControllerDelegate<NSObject, Controller>
@end

@protocol Controller<NSObject>
@end

@interface CollectionViewController ()<DataSource> {
}
@end

@interface CollectionViewController (Flow)<FlowDelegate> : NSObject
{
	NSDictionary <NSString *, NSString *> dict;
}
@end

@interface MyClass : NSObject<Protocol_A, Protocol_B>

@end

@implementation ViewController
- (void)someMethod {
	auto const *dict = [NSMutableDictionary  < NSString *, NSString * > new];
}
@end
