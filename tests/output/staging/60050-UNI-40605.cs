public string Foo =>
    "bar";
public string Foo
    => "bar";

public static T WithAdditionalFlags<T>(this T _this, IEnumerable<string> flags) where T : ObjectFileLinker
    => _this.WithLinkerSetting(l => l.Flags = l.Flags.Concat(flags));

public static T WithAdditionalFlags<T>(this T _this, IEnumerable<string> flags) where T : ObjectFileLinker =>
    _this.WithLinkerSetting(l => l.Flags = l.Flags.Concat(flags));

public static T WithAdditionalFlags<T>(this T _this, IEnumerable<string> flags) where T : ObjectFileLinker
    => _this.WithLinkerSetting(
        l => l.Flags =
            l.Flags.Concat(flags));

public static T WithAdditionalFlags<T>(this T _this, IEnumerable<string> flags) where T : ObjectFileLinker => _this.WithLinkerSetting(
    l => l.Flags =
        l.Flags.Concat(flags));

public static T WithAdditionalFlags<T>(this T _this, IEnumerable<string> flags) where T : ObjectFileLinker
    => _this.WithLinkerSetting(l =>
        l.Flags = l.Flags.Concat(flags));

public static T WithAdditionalFlags<T>(this T _this, IEnumerable<string> flags) where T : ObjectFileLinker =>
    _this.WithLinkerSetting(l
        => l.Flags = l.Flags.Concat(flags));
