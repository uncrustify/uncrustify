#include <type_traits>

static_assert(
	std::is_integral_v<T>
	&& std::is_signed_v<T>
	&& sizeof(int) >= sizeof(T),
	"msg"
	);
