EditorApplication.CallDelayed(() => {
    foreach (CollabToolbarWindow window in Resources.FindObjectsOfTypeAll<CollabToolbarWindow>())
        window.Close();
}, 1f);