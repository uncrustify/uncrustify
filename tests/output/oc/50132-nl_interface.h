

/* add case */ @interface FooIvarAdd : NSObject
{ int ivar; } @end

// remove case

@interface FooIvarRemove : NSObject
{ int ivar; } @end

// force case
@interface FooIvarForce : NSObject
{ int ivar; } @end


/* add case */ @interface FooPropAdd : NSObject
@property (nonatomic, readonly, copy) NSURL* url; @end

// remove case

@interface FooPropRemove : NSObject

@property (nonatomic, readonly, copy) NSURL* url;
@end

// force case
@interface FooPropForce : NSObject
@property (nonatomic, readonly, copy) NSURL* url;
@end

/* add case */ @interface FooScopeAdd : NSObject +(NSURL*)url;
@end

// remove case

@interface FooScopeRemove : NSObject

+(NSURL*)url;
@end

// force case
@interface FooScopeForce : NSObject
+(NSURL*)url;
@end

