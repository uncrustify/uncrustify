public static class Extensions
{
    public static FluentXboxOneSdk VS2017(this FluentPlatform<XboxOnePlatform> _) => new FluentXboxOneSdk {MsvcVersion = new Version(15, 0)};
}
