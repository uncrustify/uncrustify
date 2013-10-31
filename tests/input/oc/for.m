[[HRNewsService sharedInstance] fetchBreakingNewsItemWithId:self.breakingNewsId success:^(id responseObject) {
        NSDictionary * thing;
        for (NSArray * dictionary in photos) {
        }
} failure:^(NSError *error) {
    // Failure?
}];
