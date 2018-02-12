void (*foo)(int);
static bar(void (*foo)(int))
{
}

bool (*comp_func)(const TypeA*const a, const TypeB& value) = NULL;
static foo(bool (*comp_func)(const TypeA*const a, const TypeB& value));
static foo(bool (*comp_func)(const TypeA*const a, const TypeB& value) = NULL)
{
}

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const TypeA* lhs, const TypeB& rhs));
void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const TypeA* lhs, const TypeB& rhs) = NULL)
{
}
