#define RETURN_ON_ERROR(error) \
    if (error /* zero means no error */) \
        return;
