int short_function();
int some_very_very_very_very_very_very_very_very_long_function();
int main() {
    // short condition, no existing newlines
    if (short_function()) {}

    // short condition, existing newlines
    if (
        short_function()
        ) {}

    // long condition, no newlines
    if (some_very_very_very_very_very_very_very_very_long_function() &&
        some_very_very_very_very_very_very_very_very_long_function()) {}

    // long condition, newlines
    else if (
            some_very_very_very_very_very_very_very_very_long_function() &&
            some_very_very_very_very_very_very_very_very_long_function()
            ) {}

    // switch condition
    switch (some_very_very_very_very_very_very_very_very_long_function() &&
        some_very_very_very_very_very_very_very_very_long_function()) {
    case default: break;
    }

    // while condition, line comments
    while (
        // comment 1
        short_function()
        // comment 2
        ) {}

    // for condition, inline comments
    for ( /* a */ int i=0; some_very_very_very_very_very_very_very_very_long_function()
    && some_very_very_very_very_very_very_very_very_long_function() && i < 10; i++ // trailing comment
    ){}
}
