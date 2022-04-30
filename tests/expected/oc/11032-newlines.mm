NSString* GetXcodePath()
{
    return [[NSWorkspace sharedWorkspace]absolutePathForAppBundleWithIdentifier: kXCodeBundleId];
}

extern "C" EXPORTDLL void LaunchXCode()
{
    NSString* curApp = GetXcodePath();
    [[NSWorkspace sharedWorkspace] launchApplication: curApp];

    NSArray *selectedApps =
        [NSRunningApplication runningApplicationsWithBundleIdentifier: kXCodeBundleId];

    for (int i = 0; i < [selectedApps count]; i++)
    {
        NSRunningApplication *app = [selectedApps objectAtIndex: i];
        int count = 0;
        NSLog(@"Checking %@\n", app);
        while (![app isFinishedLaunching] && count++ < 300)
            [[NSRunLoop currentRunLoop] runUntilDate: [NSDate dateWithTimeIntervalSinceNow: 1.0f]];
    }
}

NSString* MakeNSString(const std::string& string)
{
    return MakeNSString(string.c_str());
}

NSString* MakeNSString(const char* string)
{
    NSString* ret = string ? [NSString stringWithUTF8String: string] : nil;
    return ret ? ret : @"";
}
