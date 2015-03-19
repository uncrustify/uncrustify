[self.vendorID_TextField setStringValue:string ? string : @""];

x = [NSString str:path];
x = [NSString strFormat:@"Data/%s", path];
x = path[0] == '/' ? path : "abc";
x = path[0] == '/' ? [NSString str:path] : [NSString strFormat:@"Data/%s", path];
