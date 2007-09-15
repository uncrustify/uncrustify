int foo()
{
try { foo(bar); } catch (int *e) { return 0; }

if (false) try { throw int(); } catch(...){}

if (a) { return 1; } else { return 0; }
return 1;
}

