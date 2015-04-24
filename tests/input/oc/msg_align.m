
BOOL immediatlyReady = [self ensureResource:mutableResources[0]
                    existsInDirectoryAtPath:mutablePaths[0]
                                  queueMode:mode
                          completionHandler:completionHandler
                               errorHandler:errorHandler];

[myObject doFooWith1:arg1 name1:arg2  // some lines with >1 arg
              error1:arg3];

[myObject doFooWith2:arg4
               name2:arg5 error2:arg6];

[myObject doFooWith3:arg7
          name3:arg8  // aligning keywords instead of colons
          error3:arg9];

[myObject doithereguysA:argA
 reallylongargname:argB another:argC];

 int foo()
{
[UIView transitionWithView:self.window
                  duration:0.3
                   options:UIViewAnimationOptionTransitionCrossDissolve
                animations:^{
                        BOOL oldState = [UIView areAnimationsEnabled];
                        [UIView setAnimationsEnabled:NO];
                        self.window.rootViewController = self.viewController;
                        [UIView setAnimationsEnabled:oldState];
                    }
                completion:^(BOOL finished) {

BOOL foo;
                    }];
}

 int foo2()
{
[UIView transitionWithView:self.window duration:0.3 options:UIViewAnimationOptionTransitionCrossDissolve animations:^{
                        BOOL oldState = [UIView areAnimationsEnabled];
                        [UIView setAnimationsEnabled:NO];
                        self.window.rootViewController = self.viewController;
                        [UIView setAnimationsEnabled:oldState];
                    }
                    completion:^(BOOL finished) {

BOOL foo;
                    }];

        [[HRNewsService sharedInstance] registerPushToken:deviceToken success:^{
            DLog(@"Finished Registering Push Token!");
            self.notificationsEnabled = YES;
        } fail:nil];

}
