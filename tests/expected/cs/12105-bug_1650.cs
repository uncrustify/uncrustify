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

var islands = EditorCompilationInterface.GetAllMonoIslands().Select(i => new Island
{
    MonoIsland = i,
    Name = Path.GetFileNameWithoutExtension(i._output),
    References = i._references.ToList()
}).ToList();

var projectEntries = islands.Select(i => string.Format(
    DefaultSynchronizationSettings.SolutionProjectEntryTemplate,
    SolutionGuid(i), _projectName, Path.GetFileName(ProjectFile(i)), ProjectGuid(i._output)
));


Func<IEnumerable<IMemberDefinition>, IEnumerable<IMemberDefinition>> filterMembersWithObsoleteAttr = members => members.Where(m =>
    !m.IsRuntimeSpecialName
    && !m.IsSpecialName
    && !blackList.Contains(m.FullName)
    && CheckCustomAttributes(m));
