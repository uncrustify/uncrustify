self.someErrorView = ({
    UIView *view = [[UIView alloc] init];
    view.backgroundColor = [UIColor redColor];
    [view addSubview:({
        self.someErrorLabel = ({
            UILabel *label = [[UILabel alloc] init];
            label.textAlignment = NSTextAlignmentCenter;
            label.backgroundColor = [UIColor clearColor];
            label;
        });
    })];
    view;
});
[self.view addSubview:self.someErrorView];

