// Can't set sp_inside_braces_struct=add otherwise Uncrustify starts applying it to initializers combined with old-C-style struct usage.

struct in_addr addr = {0};
// ... --> ...
struct in_addr addr = { 0 };
