class X34
{
public:
auto f01() -> bool;
auto f02() noexcept -> bool;
auto f03() noexcept(true) -> bool;
auto f04() noexcept(false) -> bool;
auto f05() noexcept -> bool = delete;
auto f06() noexcept(true) -> bool = delete;
auto f07() noexcept(false) -> bool = delete;

auto f11() const -> bool;
auto f12() const noexcept -> bool;
auto f13() const noexcept(true) -> bool;
auto f14() const noexcept(false) -> bool;
auto f15() const noexcept -> bool = delete;
auto f16() const noexcept(true) -> bool = delete;
auto f17() const noexcept(false) -> bool = delete;

auto f21() throw() -> bool;
auto f22() throw() -> bool = delete;
auto f23() const throw() -> bool;
auto f24() const throw() -> bool = delete;
};
