typedef struct Mixed
{
    int      count;
    uint32_t (*AddRef)(IMUIObject *Self);
    char     *name;
    void     (*Shutdown)(IMUIObject *Self);
} Mixed;
