
NSAssert([self.delegate conformsToProtocol: @protocol(UISearchBarDelegate)], @"Some Error.");

Protocol *counter = @protocol(ReferenceCounting);

@protocol ReferenceCounting

-setRefCount: (int)count;

-(int)refCount;

-incrementCount;

-decrementCount;

@end

@interface Formatter : NSObject<Formatting, Prettifying>

@end

if ([receiver conformsTo: @protocol(ReferenceCounting)])
{
   [receiver incrementCount];
}

@protocol B;

@protocol A
-Foo: (id<B>)anObject;
@end
