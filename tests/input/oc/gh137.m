[UIView animateWithDuration:0.2f delay:0.f options:UIViewAnimationCurveEaseInOut animations: ^{
    self.transform = CGAffineTransformMakeScale(1.05f, 1.05f);
} completion: ^(BOOL finished) {
    [UIView animateWithDuration:0.08f delay:0.f options:UIViewAnimationOptionCurveEaseInOut animations: ^{
            self.transform = CGAffineTransformIdentity;
            [UIView animateWithDuration:1 delay:0 options:0 animations:^   {
                    // blah
                } completion:nil];
        }];
}];

dispatch_async(foo, ^{
    dispatch_async(bar, ^{
            dispatch_async(qux, ^{
                    quz();
                });
        });
})
