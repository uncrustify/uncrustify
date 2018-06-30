NSString *str = (otherString ?: @"this is the placeholder");
NSString *str2 = (str ? otherString : @"this is the other placeholder");
NSString *str3 = str ? [[NSString alloc] initWithString:str] : @"this is the third placeholder";
id str4 = str ? [self methodWithParameter1:@{@"bla": ({[self anotherMethod:@{@"id": @1}];})}
                 andParameter2:@{@"dict_key": @{@"nested_dict_key_1": @(1), @"nested_dict_key_2": @"colon:in:string"}}] : [self anotherMethod:str ? @1 : @2];