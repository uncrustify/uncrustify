auto lam = []<typename T>() mutable -> void
           {
           };
auto lam2 = [a = 42]<typename T>() mutable -> void
            {
            };
