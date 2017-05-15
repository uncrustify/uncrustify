// Make sure browser-side code finishes executing before we kill the window
EditorApplication.CallDelayed(() => {
    foreach (CollabToolbarWindow window in Resources.FindObjectsOfTypeAll<CollabToolbarWindow>())
        window.Close();
}, 1f);