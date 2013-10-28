NSString *str = (otherString ?: @"this is the placeholder");
NSString *str2 = (str ? otherString : @"this is the other placeholder");
