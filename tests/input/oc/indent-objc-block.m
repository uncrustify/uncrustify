const auto c =
  [FDSTapTargetComponent
   accessibilityContext:{
     .accessibilityLabel = ^{
		   return [AccessibilityLabelBuilder build];
		 }
   }];

methodCall1(^{
  send(component1);
},
           x);

methodCall2( ^ {
  send(component2);
          });

[array block:^ (id obj, NSUInteger idx, BOOL *stop) {
        NSLog(@"Object at index %lu is %@", idx, obj);
    }];


    [UIView animateWithDuration:3.0f animation:^{
      LOG(@"animate");
    }
    completion:^(BOOL finished){
      LOG(@"finished");
    }];

[UIView
     animationBlock: ^ {
      [[Log alloc] callback:^NSString *(NSString *result){
        return @"log";
      }];
    }
    completion:^(BOOL finished){
      LOG(@"finished");
    }];


methodCall3(x, ^KSC::ActionCell::Item (Item item) {
              variant.action.send(component);
      });

methodCall4(  x, ^ id (Component *c) {
                                NSLog(@"methodCall4");
                              });

methodCall5(  ^ id (Component *c) {
                                NSLog(@"methodCall5");
                              });

methodCall6(  ^(NSString *)(Component *c) {
                             return @"methodCall6";
                           });

methodCall7(^ (Component *c) {
                             NSLog(@"methodCall7");
                           }, y);

  methodCall8(x,  ^(Component *c) {
                             NSLog(@"methodCall8");
                           }, y);


                           [Object callMethod:xArg
                           block:^id (Component *c) {
                                NSLog(@"methodCall4");
                           }];

[Object callMethod:xArg
                           block:^id (Component *c) {
                                NSLog(@"methodCall5");
                              }];

[Object callMethod:xArg block:^(NSString *)(Component *c) {
                             return @"methodCall6";
                           }];


[Object callMethod:xArg
             block:^ (Component *c) {
                             NSLog(@"methodCall7");
                           }
                           yMethod:yArg];

  [Object callMethod:xArg
  block:^(Component *c) {
                             NSLog(@"methodCall8");
                           }
                           yMethod:yArg];

[Object callMethod:xArg
block:^(NSString *)(Component *c1) {
                             [Object callMethod:xArg block:^(Component *c) {
                             NSLog(@"methodCal9");
                           }
                           yMethod:yArg];
                           }
                           anotherBlock:^(NSString *)(Component *c2) {
                             return @"methodCall10";
                           }
                           yetAnotherBlock:^(NSString *)(Component *c3) {
                             return @"methodCall11";
                           }];

[dialog
        dismissWithCompletion:^{
          _deleteConversation(
              strongSelf->_session,
          ^{
        if (auto const innerStrongSelf = weakSelf) {
              [NavigationCoordinator(innerStrongSelf)
              dismissViewController:innerStrongSelf
              completion:nil];
            }
            });
      }];


      [FlexboxComponent
newWithView:{
   {
  .accessibilityLabel = ^{
    return
  [[[[AccessibilityLabelBuilder builder]
      appendPhrase:title]
    appendPhrase:body]
    getResult];
    },
   }
 }];


MainComponent(
  .builder = ^{
  return
  value;
},
param1,
param2,
  );

KSC::map(
  _items,
      ^ ActionCell::Item (Item item)  {
     return x;
          }
);
