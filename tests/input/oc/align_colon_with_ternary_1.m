static void func() {
    [object test:NO ? PARAM1 : PARAM2
     test2:YES];
}

static void func() {
    [object test:NO ? PARAM1 : PARAM2
    test2:YES
           test3:YES];
}

static void func() {
    [object test:NO ? PARAM1 : PARAM2
    test2:YES
           test3:YES ? PARAM1 : PARAM2
           test4:NO];
}

static void func() {
    [object test:NO ? PARAM1 : YES
    ? nil:nil
    test2:YES
           test3:YES ? PARAM1 : PARAM2
           test4:NO];
}

// Note: This is an extreme case and in the future we may add rules that update this formatting
static void func() {
    NSString *s = YES ? [object test:NO ? PARAM1 : YES
    ? nil:nil
    test2:YES
           test3:YES ? PARAM1 : PARAM2
           test4:NO] : [object test:NO ? PARAM1 : YES
    ? nil:nil
    test2:YES
           test3:YES ? PARAM1 : PARAM2
           test4:NO];
}

static void func() {
	NSString *s = YES ? [object test:NO ? PARAM1 : YES ? nil : nil
	                           test2:YES
	                           test3:YES ? PARAM1 : PARAM2
	                           test4:NO] : [object test:NO ? PARAM1 : YES ? nil : nil
	                                              test2:YES
	                                              test3:YES ? PARAM1 : PARAM2
	                                              test4:NO];
}
