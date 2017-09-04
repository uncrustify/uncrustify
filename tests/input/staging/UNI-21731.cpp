template<class R, typename ... FArgs>
struct invoke_fptr<R(VKAPI_PTR*)(FArgs...)>
{
};
