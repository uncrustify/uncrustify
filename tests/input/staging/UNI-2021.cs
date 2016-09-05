// Non-indented two-line func+body
// Test code:

int Func()
    { return 0; }

// The contents of the method will get aligned with 'int'. Sometimes we want this, sometimes we don't.

// There is no workaround; would require a new Uncrustify feature.
// Re-indentation of wrapped args leaves hanging previous-line args
// This:

// ```cs

UtilityFunctions.DisplayAwesomeness (position, fullFlagNames.ToArray (),
                                 // Only show selections if we are not multi-editing
                                 GUI.showMixedValue ? new int[] {} : selectedFlags.ToArray (),
                                 Boo.m_Instance.Foo, null);
// ```
// becomes this:

// ```cs

UtilityFunctions.DisplayAwesomeness(position, fullFlagNames.ToArray(),
    // Only show selections if we are not multi-editing
    GUI.showMixedValue ? new int[] {} : selectedFlags.ToArray(),
    Boo.m_Instance.Foo, null);

// ```

// but we'd rather have this:

// ```cs

UtilityFunctions.DisplayAwesomeness(
    position, fullFlagNames.ToArray(),
    // Only show selections if we are not multi-editing
    GUI.showMixedValue ? new int[] {} : selectedFlags.ToArray(),
    Boo.m_Instance.Foo, null);

// ```
