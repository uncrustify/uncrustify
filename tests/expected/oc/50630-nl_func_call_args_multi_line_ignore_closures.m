mapToPtr(^(const LeftAddOn::Props &addOnProps) {
	FSTheme *const theme = AK::getTheme();
});

mapToPtr( x, ^ (const Props &addOnProps) {
	FSTheme *const theme = AK::getTheme();
});

mapToPtr( ^ (const Props &addOnProps) {
	FSTheme *const theme = AK::getTheme();
});

mapToPtr(
	arg1, ^ ( NSString * ) (const Props &addOnProps) {
	FSTheme *const theme = AK::getTheme();
}, arg2
	);

mapToPtr(arg1, ^ ( NSString *) (const Props &addOnProps) {
	FSTheme *const theme = AK::getTheme();
});

mapToPtr( ^() (const Props &addOnProps) {
	FSTheme *const theme = AK::getTheme();
}, arg2);



methodCall(^{
	variant.action.send(Cmpnt);
});

methodCall(
	^{
	variant.action.send(Cmpnt);
}, x);


methodCall(  x, ^id (Cmpnt *c) {
	NSLog(@"Something");
});

methodCall(  ^id (Cmpnt *c) {
	NSLog(@"Something");
});

methodCall(  ^(Cmpnt *c) {
	NSLog(@"Something");
});

methodCall(
	^ (Cmpnt *c) {
	NSLog(@"Something");
}, y);

methodCall(
	x,  ^(Cmpnt *c) {
	NSLog(@"Something");
}, y
	);


methodCall(
	arg1,
	arg2,
	arg3
	);

methodCall(arg1, arg2, arg3);

methodCall(
	arg1,
	arg2, {
	.x = 10,
}
	);

methodCall(
	arg1, {
	.x = 10,
},
	arg2
	);

methodCall({
	.x = 10,
},
           arg2);


outerMethodCall(
	methodCall(^{
	// action
},
	           x)
	);

outerMethodCall(
	methodCall(^{
	variant.action.send(Cmpnt);
},
	           x)
	);
