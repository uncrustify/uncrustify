private static Type[] GetAllVisualElementTypes()
{
    return typeof(VisualElement).Assembly.GetTypes()
        .Where(t => t != typeof(VisualElement) &&
                t != typeof(Panel) &&
                !t.IsAbstract &&
                !typeof(IMElement).IsAssignableFrom(t) &&
                !typeof(IMContainer).IsAssignableFrom(t) &&
                typeof(VisualElement).IsAssignableFrom(t)).ToArray();
}

// to this
private static Type[] GetAllVisualElementAssetTypes()
{
    return typeof(VisualElement).Assembly.GetTypes()
        .Where(t => t != typeof(VisualElement) &&
            t != typeof(Panel) &&
            !t.IsAbstract &&
            !typeof(IMElement).IsAssignableFrom(t) &&
            !typeof(IMContainer).IsAssignableFrom(t) &&
            typeof(VisualElement).IsAssignableFrom(t)).ToArray();
}
