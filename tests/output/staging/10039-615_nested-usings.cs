if (NewBuildCodeEnabled.ForMacStandalone)
    using (TinyProfiler.Section("MacStandaloneSupport.Setup"))
        new MacStandaloneSupport().Setup();

using (var x = X())
using (var y = Y())
{
}

using (var x = X())
using (var y = Y())
{
}
