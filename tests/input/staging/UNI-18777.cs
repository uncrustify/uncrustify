// I want to keeep the function call indented
Thing
    .Select(x => x > 2)
    .ToList();

var x = Thing
    .Select(x => x > 2)
    .ToList();