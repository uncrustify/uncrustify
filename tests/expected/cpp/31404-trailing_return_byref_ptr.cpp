struct Foo
{
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

	auto f25() const -> bool *;
	auto f26() const -> bool * = delete;
	auto f27() const -> bool &;
	auto f28() const -> bool & = delete;

	auto f29() -> bool *;
	auto f30() -> bool * = delete;
	auto f31() noexcept -> bool *;
	auto f32() noexcept(true) -> bool *;
	auto f33() noexcept(false) -> bool *;
	auto f34() noexcept -> bool * = delete;
	auto f35() noexcept(true) -> bool * = delete;
	auto f36() noexcept(false) -> bool * = delete;
	auto f37() throw() -> bool *;
	auto f38() throw() -> bool * = delete;

	auto f39() -> bool &;
	auto f40() -> bool & = delete;
	auto f41() noexcept -> bool &;
	auto f42() noexcept(true) -> bool &;
	auto f43() noexcept(false) -> bool &;
	auto f44() noexcept -> bool & = delete;
	auto f45() noexcept(true) -> bool & = delete;
	auto f46() noexcept(false) -> bool & = delete;
	auto f47() throw() -> bool &;
	auto f48() throw() -> bool & = delete;

	auto f49() const -> bool *;
	auto f50() const -> bool * = delete;
	auto f51() const noexcept -> bool *;
	auto f52() const noexcept(true) -> bool *;
	auto f53() const noexcept(false) -> bool *;
	auto f54() const noexcept -> bool * = delete;
	auto f55() const noexcept(true) -> bool * = delete;
	auto f56() const noexcept(false) -> bool * = delete;
	auto f57() const throw() -> bool *;
	auto f58() const throw() -> bool * = delete;

	auto f59() const -> bool &;
	auto f60() const -> bool & = delete;
	auto f61() const noexcept -> bool &;
	auto f62() const noexcept(true) -> bool &;
	auto f63() const noexcept(false) -> bool &;
	auto f64() const noexcept -> bool & = delete;
	auto f65() const noexcept(true) -> bool & = delete;
	auto f66() const noexcept(false) -> bool & = delete;
	auto f67() const throw() -> bool &;
	auto f68() const throw() -> bool & = delete;

	class Foo
	{
	auto operator=(const Foo &) -> Foo &;
	auto operator=(const Foo &) noexcept -> Foo &;

	auto operator=(Foo const &) -> Foo &;
	auto operator=(Foo const &) noexcept -> Foo &;

	auto operator=(Foo &&) -> Foo &;
	auto operator=(Foo &&) noexcept -> Foo &;
	}
};
