void foo()&;
void foo()&&;

void bar() const&;
void bar() const&&;

void baz() noexcept&;
void baz() noexcept&&;

void complex() const noexcept&;
void complex() const noexcept&&;

void func()&{
}

void func()&&{
}

void getValue() const&-> int;
void getValue()&&-> int;

std::vector<int>&& withTemplate()&;
std::vector<int>&& withTemplate()&&;

[[nodiscard]] ns::myClass<int>&& withTemplate()&;
[[nodiscard]] ns::myClass<int>&& withTemplate()&&;

[[nodiscard]] ns::myClass<nested::otherClass&&>&& withNestedTemplate()&;
[[nodiscard]] ns::myClass<nested::otherClass&&>&& withNestedTemplate()&&;

std::map<std::string, std::string>&& multiParam()&;
std::map<std::string, std::string>&& multiParam()&&;

[[nodiscard]] std::map<std::string, std::string, std::less<> >&& multiParamWithTemplate()&;
[[nodiscard]] std::map<std::string, std::string, std::less<> >&& multiParamWithTemplate()&&;
