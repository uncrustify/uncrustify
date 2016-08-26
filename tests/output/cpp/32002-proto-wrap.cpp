WRAP_FUNCTION(Foo, Bar& (void));
WRAP_FUNCTION(Foo, Bar* (void));
WRAP_FUNCTION(Foo, (Bar& (void)));
WRAP_FUNCTION(Foo, (Bar* (void)));

WRAP_FUNCTION(Foo, int (Bar&));
WRAP_FUNCTION(Foo, int (Bar*));
WRAP_FUNCTION(Foo, (int (Bar&)));
WRAP_FUNCTION(Foo, (int (Bar*)));
