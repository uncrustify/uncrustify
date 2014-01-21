[[HRNewsService sharedInstance] fetchBreakingNewsItemWithId:self.breakingNewsId success:^(id responseObject) {
        NSDictionary * thing;
        for (NSArray * dictionary in photos) {
        }
} failure:^(NSError *error) {
    // Failure?
}];

// We also need to consider cases where a non-pointer type (or, a pointer without the star) is declared in ObjC.
for (id obj in someDictionary) {
    NSLog(@"This could be anything! Objective-C really needs parametrized collections.");
}
