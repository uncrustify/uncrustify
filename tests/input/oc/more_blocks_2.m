// Test a case where blocks wrapped by parentheses were causing the parser to crash
int (^myBlock)(int) = ( ^(int num) {
    return num * multiplier;
});

dispatch_async(thread, (^{
    dispatch_async(thread, ^{
        dispatch_async(thread, ^{
            NSLog(@"Hooray for dispatch_async!");
        });
    });
}));

// Example of a unit test using Kiwi
beforeAll(^{
    NSString *serviceURL = [NSURL URLWithString:@"http://TEST_URL"];
    NSLog(serviceURL);
});
