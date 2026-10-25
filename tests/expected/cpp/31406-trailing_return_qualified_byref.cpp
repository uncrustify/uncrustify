auto f01() -> ns::Bar &;
auto f02() -> ns::Bar *;
auto f03() -> a::b::Bar &;
auto f04() -> ::Bar &;
auto f05(const std::int16_t &) -> std::int16_t &;
auto f06() noexcept -> std::int16_t &;

struct Foo
{
	auto f07() const -> ns::Bar &;
	auto operator=(const ns::Bar &) -> ns::Bar &;
};
