// *Single line functions*

public class Class
{
    public string foo {get; set; }

    bool HasBar() {return m_HasBar != 0; }

    public Bar prop {get {return m_bar; } set {m_bar = value; }}

// This seems to happen with no spaces on the interior. Opening brace doesn't get one, closing brace does.

// Turning on sp_inside_braces=add fixes it, but also changes a lot of initializer code we don't want to touch (like x = {1}). May need special support, or perhaps there's a bug..

// *Initializers*

// Not sure if this is what we want..

    public void foo()
    {
        sas.Foo("bar", new Dictionary<string, object>(){ { "k1", "v1" }, { "k2", "v2" } });
        // ... --> ...
        sas.Foo("bar", new Dictionary<string, object>() { { "k1", "v1" }, { "k2", "v2" } });
    }

// Second line adds a space before the initializer {. Is that what we want for C#?
}