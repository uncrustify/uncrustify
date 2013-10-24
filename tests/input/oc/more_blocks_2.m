// Test a case where blocks wrapped by parentheses were causing the parser to crash
int (^myBlock)(int) = ( ^(int num) {
    return num * multiplier;
});

dispatch_async(thread, (^{
    NSLog(@"Hooray for dispatch_async!");
}));
