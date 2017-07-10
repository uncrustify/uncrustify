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

private static Type[] GetAllVisualElementAssetTypes()
    {
        return typeof(VisualElementAsset).Assembly.GetTypes()
            .Where(t => t != typeof(VisualElementAsset) &&
                    !t.IsAbstract &&
                    typeof(VisualElementAsset).IsAssignableFrom(t)).ToArray();
    }

//We would like to get something like
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

// Or at least
return typeof(VisualElement).Assembly.GetTypes()
    .Where(t => t != typeof(VisualElement) &&
        t != typeof(Panel) &&
        !t.IsAbstract &&
        !typeof(IMElement).IsAssignableFrom(t) &&
        !typeof(IMContainer).IsAssignableFrom(t) &&
        typeof(VisualElement).IsAssignableFrom(t)).ToArray();
