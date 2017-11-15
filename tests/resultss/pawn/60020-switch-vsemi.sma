public plugin_init()
{
    new i=5;

    switch(i) {
    case 3: return false;
    case 5:
    {
        i = 6;
        return true;
    }
    default: {
        return true;
    }
    }
}