- (void)dealloc {
    [self closeFile];

    [self setData:nil];

    [super dealloc];
}
