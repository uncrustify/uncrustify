// I want to keeep the function call indented
Thingy
    .Select(x => x > 2)
    .ToList();

// it works with a var
var x = Thingy
    .Select(x => x > 2)
    .ToList();
