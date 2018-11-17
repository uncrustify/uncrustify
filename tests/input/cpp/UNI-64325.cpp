class function_ref
{
public:
    template<typename CallableT>
    function_ref(CallableT &&t) noexcept
        : m_Ptr((void *)std::addressof(t))
        , m_ErasedFn([](void *ptr, Args... args) -> ReturnValue
        {
            // Type erasure lambda: cast ptr back to original type and dispatch the call
            return (*reinterpret_cast<std::add_pointer_t<CallableT>>(ptr))(std::forward<Args>(args)...);
        })
    {}
};
