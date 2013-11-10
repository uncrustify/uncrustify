

/* add case */ @implementation FooIvarAdd { int ivar; } @end

// remove case
@implementation FooIvarRemove { int ivar; } @end

// force case
@implementation FooIvarForce { int ivar; } @end


/* add case */ @implementation FooScopeAdd +(NSURL*)url {
	return nil;
}
@end

// remove case
@implementation FooScopeRemove

+(NSURL*)url {
	return nil;
}
@end

// force case
@implementation FooScopeForce
+(NSURL*)url {
	return nil;
}
@end

