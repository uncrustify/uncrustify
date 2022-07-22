static void func() {
    [object test:NO ? PARAM1 : PARAM2 test2:YES];
}


static void func() {
    [object test:NO ? PARAM1 : PARAM2 test2:YES test3:YES];
}

static void func() {
    [object test:NO ? PARAM1 : PARAM2 test2:YES test3:YES ? PARAM1 : PARAM2 test4:NO];
}

static void func() {
    [object test:NO ? PARAM1 : YES ? nil:nil test2:YES test3:YES ? PARAM1 : PARAM2 test4:NO];
}

static void func() {
	NSString *s = YES ? [object test:nil test2:YES test3:nil test4:NO] : nil;
}
