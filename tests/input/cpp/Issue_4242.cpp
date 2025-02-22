auto lam = []<typename T>() mutable -> void
           {
           };
auto lam2 = [a = 42]<typename T>() mutable -> void
            {
            };
auto lam3 = [a = 42]<typename T>() mutable constexpr consteval noexcept noexcept(true) noexcept(noexcept(true)) -> void
            {
            };
