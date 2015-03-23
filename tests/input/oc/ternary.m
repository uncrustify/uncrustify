NSString *str = (otherString ?: @"this is the placeholder");
NSString *str2 = (str ? otherString : @"this is the other placeholder");
NSString *str3 = str ? [[NSString alloc] initWithString:str] : @"this is the third placeholder";
