// Singular with various newline formats
auto f = [] -> void {
    return;
};

auto f = [] -> void {
    return;
}();

auto f = [] -> void
{
    return;
}();

auto f = 
[] -> void {
    return;
};

auto f = 
[] -> void
{
    return;
};

auto f
    = [] -> void {
        int i = 0;
        return;
    };

auto f
    = []
      {
          int i = 0;
          return;
      };

// Nested lambda
auto f = [] {
    auto g = [] {
        auto h = [] {
            return;
        };
        return;
    };
    return;
};

auto f = [] {
    auto g = []
    {
        auto h = [] {
            return;
        };
        return;
    };
    return;
};

auto f = []
{
    auto g = [] {
        auto h = []
        {
            return;
        };
        return;
    };
    return;
};

// Nested lambda within functions
Func(
    [] { return; },
    [] { return; }
);

Func([] { return; },
    [] { return; }
);

Func([] { return; },
    [] { return; }
)();

Func([] { return; },
    [] { return; })();

Func([] { return; },
    [] { return; });

A(
        B([] (const std::string &s) -> bool {
            s = "hello";
            return true;
            }), 1
        );

A(
        B(
            [] (const std::string &s) -> bool {
            s = "hello";
            return true;
            }
            ), 1
        );

std::thread([](const char *c) {
    std::cout << c << std::endl;
}).detach();

Func(std::count_if(v.begin(), v.end(), [&](const auto &a) {
            return a == 3;
    }));

Func(
        std::count_if(v.begin(), v.end(), [&](const auto &a)
            {
            return a == 3;
    }));

Func(
        std::count_if(v.begin(), v.end(), [&](const auto &a) {
            return a == 3;
    }));

Func(
        std::count_if(v.begin(), v.end(), [&](const auto &a) {
            return a == 3;
    })
    );

// Test case from issue #3116
const auto compare = [] (const auto i, const auto j)
{
    return i >= j;
};

std::sort(
    vector.begin(),
    vector.end(),
    [] (const auto i, const auto j)
{
    return i >= j;
}
);

// Test case from issue #3116
if(isWidgetOfCurrentRow)
{
    it = std::find_if(
            reloaded.begin(),
            reloaded.end(),
            [&rowGuid](const auto& device)
            {
            return (device.thingGUID == rowGuid && !device.isWidget);
            }
            );
}
else
{
    it = std::find_if(
            reloaded.begin(),
            reloaded.end(),
            [&rowGuid](const auto& device)
            {
            return device.thingGUID == rowGuid;
            }
            );
}
