void test(NSString *             param1, void (^_Nullable completionBlock)(int));

void test(NSString * param1, void (^_Nullable completionBlock)(int));

void test(void (^_Nullable completionBlock)(int), NSString *param1);

void test(void (^_Nullable completionBlock)(int), NSString *param1, void (^_Nullable completionBlock)(int));

void test(
	void (^_Nullable completionBlock)(int),
	NSString * param1,
	void (^_Nullable completionBlock)(int)
	);

void test(NSString *param1, void (^_Nullable completionBlock)(NSString *, NSString *, void (^_Nullable completionBlock2)(int)));
