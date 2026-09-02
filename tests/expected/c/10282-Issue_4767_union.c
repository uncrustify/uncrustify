typedef union UShell
{
    uint32_t (*AddRef)(IMUIObject *Self);
    int      raw;
} UShell;
