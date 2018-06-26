
@interface EmptyClass : NSObject
-(void)aMessage: (id) arg;
@end

@interface EmptyClass : NSObject
{
}
-(void)aMessage: (id) arg;
@end

@interface NSObject (ObjectAdditions)
-(void)aMessage: (id) arg;
@end

@protocol TestProtocol
-(void)aMessage: (id) arg;
@end

@interface TestClass : NSObject<TestProtocol>
{
}
-(void)aMessage: (id) arg;
@end
