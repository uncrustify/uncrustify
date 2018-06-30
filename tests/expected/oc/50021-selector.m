@selector (methodNameWithArg:);
@selector (methodNameNoArg);
@selector (methodNameArg1:arg2:);

NSArray *sortedTZs = [[NSTimeZone knownTimeZoneNames]
                      sortedArrayUsingSelector : @selector (compare:)];
