
#define m_new(type, num) ((type *)(m_malloc(sizeof(type) * (num))))
void *m_malloc(size_t num_bytes);
