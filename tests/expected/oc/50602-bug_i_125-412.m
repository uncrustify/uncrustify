[[NSFileManager defaultManager] createFileAtPath:path
                                        contents:data
                                      attributes:nil]

[self.myProperty setObject: obj forKey: key];

NSString *newValue = [@"my string" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];

NSString *newValue = [myStrings[0] stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
