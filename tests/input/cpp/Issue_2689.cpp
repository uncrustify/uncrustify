class C
{
public:
   size_t f4 : 8 * sizeof(size_t) - 2; // <-- this star is treated a pointer token
};
