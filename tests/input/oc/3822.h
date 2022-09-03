@interface FooInterface : UIViewController

@property (nonatomic, readwrite, weak) id<FooProtocol> delegate;

@end
