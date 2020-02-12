
void Events1(NSString *    identifier, void (^handler)());

void Events2(NSString *    identifier, void (^)());

@implementation NSArray (WWDC)
- (NSArray *)map:(id (^)(id))xform {
	id result = [NSMutableArray array];
	for (id elem in self)
		[result addObject:xform(elem)];
	return result;
}

- (NSArray *)collect:(BOOL ( ^ )(id))predicate {
	id result = [NSMutableArray array];
	for (id elem in self)
		if (predicate(elem))
			[result addObject:elem];
	return result;
}

- (void)each:(void (^)(id object))block {
	[self enumerateObjectsUsingBlock:^ (id obj, NSUInteger idx, BOOL *stop) {
	                                         block(obj);
					 }];
}

// corner case: block literal in use with return type
id longLines = [allLines collect:^ BOOL (id item) {
                                         return [item length] > 20;
				 }];

// corner case: block literal in use with return type
id longLines = [allLines collect:^ BOOL* (id item) {
                                         return [item length] > 20;
				 }];

@end

nestedMethodCall(methodCall(^ BOOL * (id item) {
	NSLog(@"methodCall")
}));

nestedMethodCall(
	arg1,
	methodCall(^ NSString * (id item) {
	NSLog(@"methodCall")
}));

nestedMethodCall(
	arg1,
	methodCall(^ {
	NSLog(@"methodCall")
},
	           arg2)
	);

nestedMethodCall(
	methodCall(^ {
	NSLog(@"methodCall")
})
	);

// 1. block literal: ^{ ... };
// 2. block declaration: return_t (^name) (int arg1, int arg2, ...) NB: return_t is optional and name is also optional
// 3. block inline call ^ return_t (int arg) { ... }; NB: return_t is optional
