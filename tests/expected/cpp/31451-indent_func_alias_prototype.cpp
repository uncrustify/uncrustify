
using Fun1 = void (  );
using Fun2 = void (  )   noexcept;

using Fun1a = void (
                   );

using Fun2a = void (
                   )   noexcept;

using Fun3a = void (
	int a,
	const char*
                   );

using Fun4a = void (
	int a,
	const char*
                   ) noexcept;

using Fun5a = void (
	int a,
	const char*
                   );

using Fun6a = void (
	int a,
	const char*
                   ) noexcept;

using Fun1b = auto (
                   )->int;

using Fun2b = auto (
                   )   noexcept->int;

using Fun3b = auto (
	int a,
	const char*
                   )->int;

using Fun4b = auto (
	int a,
	const char*
                   ) noexcept->int;

using Fun5b = auto (
	int a,
	const char*
                   )->int;

using Fun6b = auto (
	int a,
	const char*
                   ) noexcept->int;
