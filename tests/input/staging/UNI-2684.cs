EditorApplication.CallDelayed(() => {
foreach (CollabToolbarWindow window in Resources.FindObjectsOfTypeAll<CollabToolbarWindow>())
    window.Close();
}, 1f);


var requestedTargets = storage.GlobalVariable("JAM_COMMAND_LINE_TARGETS").Elements.Select(e =>
{
    var t = jamState.GetTarget(e);
    if (t == null)
        throw new Exception($"Unknown target '{e}' while writing build dependency graph.");
    return t;
}).ToArray();