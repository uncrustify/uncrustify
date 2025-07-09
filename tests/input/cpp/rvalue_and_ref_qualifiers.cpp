// ref-qualifiers

void foo() &;
void foo()&&;

void bar() const &;
void bar() const&&;

void baz() noexcept &;
void baz() noexcept&&;

void complex() const noexcept &;
void complex() const noexcept&&;

void func() &{
}

void func()&&{
}

void getValue() const &->int;
void getValue()&&->int;

std::vector<int>&&withTemplate() &;
std::vector<int>&&withTemplate()&&;
[[nodiscard]] ns::myClass<int>&&withTemplateClass() &;
[[nodiscard]] ns::myClass<int>&&withTemplateClass()&&;
[[nodiscard]] ns::myClass<nested::otherClass&&>&&withNestedTemplate() &;
[[nodiscard]] ns::myClass<nested::otherClass&&>&&withNestedTemplate()&&;
std::map<std::string, std::string>&&multiParam() &;
std::map<std::string, std::string>&&multiParam()&&;

[[nodiscard]] std::map<std::string, std::string, std::less<> >&&multiParamWithTemplate() &;
[[nodiscard]] std::map<std::string, std::string, std::less<> >&&multiParamWithTemplate()&&;

// bool in conditions

for(auto x = true; x&&y; y++)
{}

if(a&&b)
{}

if(!(a&&b))
{}

if(auto foo = 42; a&&b)
{}

if(int&&x = getValue(); x && 0)
{}

// chained method calls

void chainedMethodTest()
{
   if (  ptr->func(42)
      && !ptr->otherFunc(21)
      && (prev = ptr->getStuff())->isValid())
   {
   }
   while (  check()
         && (result = compute())->isReady()
             && funcCall())
   {
   }
   return(  isEnabled()
         && (  isReady()
            || isDone()));
}

// rvalue params

void func(int&&param);
void func2(std::vector<int>&&vec);
void func2(std::string&& s, std::vector<int&&>&&);
void func3(const int&& x);
void func4(int&& a, double&& b);
void func5(int& lref, int&& rref);

// variables and typedefs

int&&rval1 = 42;
std::string&&rval2 = getValue();
auto&&rval3 = getValue();

template<typename T>
void forward(T&&param);

template<int N> struct typeT { };
typeT<42>&&var1 = getValue();
std::vector<int>&&var2 = getValue();

typedef int&&IntRRef;
using StringRRef = std::string&&;

int&&x = static_cast<int&&>(42);

// bool ops in templates

void mixed() {
    if (a&&b&&condition())
    {
        int&&temp = std::move(value);
    }
}

template<class B1 = void, class B2 = void>
struct conjunction : bool_constant<B1::value1&&B2::value2> {};

template<bool B>
struct enableIfBool : std::enable_if<B&&true> {};

template<typename T>
using isValid = std::bool_constant<std::is_class_v<T>&&std::is_move_constructible_v<T>>;

template<typename T>
class Container {
    T&&get();
    void set(T&&value);
};

template<>
class Container<int&&> {
    int&&value;
};

// lambdas

auto lambda1 = [](int&&x) { return x; };
auto lambda2 = [x = std::move(value)]() mutable -> int&&{ return std::move(x); };
auto lambda3 = [](int&& x, std::tuple<int&&, std::tuple<int&&, std::vector<std::string&&>>>&&) { return x; };

// return types

int&&returnRvalue();
template<typename T> T&&returnForwarding(T&&t);
const int&&constRvalueReturn();
template<typename T>
std::vector<int&&>&& func8(T&& x);
template<typename T, typename U>
std::tuple<int&&, std::vector<int&&>&&>&& func9(T&& a, U&& b);

int&&ref = (x > 0) ? static_cast<int&&>(a) : static_cast<int&&>(b);
auto ternaryTemplate1 = (a&&foo<int>()) ? 1 : 2;
auto ternaryTemplate2 = (a&&bar<int, float>()) ? 1 : 2;
auto ternaryTemplate3 = (check()&&validate<T>()) ? doSomething() : doOther();

for (auto&&item : container) {}
for (int&&val : getTemporaryVector()) {}

// member pointers, cv-qual

using MemberPtr = int MyClass::*;
using RvalueRefMemberFunc = void (MyClass::*)(int&&);

bool result1 = a&&b&&c&&d;
void multiParam(int&&a, double&&b, std::string&&c);
if (expr1&&expr2) {
    Type&&temp = getValue();
}

const int&&constRval = 42;
volatile int&&volatileRval = getValue();

class MyClass {
    MyClass& operator=(MyClass&&other);
    MyClass operator+(MyClass&&other);
};

class MyClass2 {
	friend void swap(MyClass2&&a, MyClass2&&b);
};

template<typename... Args>
void variadicForward(Args&&... args);

// trailing return

auto func11() -> std::tuple<int&&, std::tuple<int&&, std::vector<std::string&&>>>&&;
auto func12(int&& x, std::tuple<int&&, std::tuple<int&&, std::vector<std::string&&>>>&&) -> int&&;

class MyClass3 {
public:
    void method1(int&& x);
    void method2() &&;
    void method3() const &&;
    MyClass3(MyClass3&& other);
    MyClass3& operator=(MyClass3&& other);
};

class DeletedMoveClass {
public:
    DeletedMoveClass(DeletedMoveClass&& other) = delete;
};

class DefaultedMoveClass {
public:
    DefaultedMoveClass& operator=(DefaultedMoveClass&& other) = default;
};

// function pointers

int&& (*simpleFp)(int&&);
std::tuple<int&&, std::tuple<int&&, std::vector<std::string&&>>>&& (*funcPtr1)(int&&);
int&& (*funcPtr2)(int&&, std::tuple<int&&, std::tuple<int&&, std::vector<std::string&&>>>&&);
std::vector<int&&>&& (*nestedFp)(std::tuple<int&&>&&);
int&& (*fpArray[10])(int&&);
int&& (**fpPtr)(int&&);
void takesFp(int&& (*cb)(int&&));
void takesFp_template(std::vector<int>&&(*cb)(std::string&&));
int&& (*returnsFp())(int&&);
std::vector<int>&&(*returnsFp_template())(std::string&&);
std::function<void(int&&, std::tuple<int&&, std::tuple<int&&, std::vector<std::string&&>>>&&)> funcObj1;

using FuncType = void(int&&);
typedef void (*FuncPtr)(int&&);
using FuncPtrReturnRvalue = int&&(*)(int&&);
using FuncPtrReturnRvalueTemplate = std::vector<int>&&(*)(std::string&&);
using FuncPtrMultiParam = std::pair<int&&, std::string&&>&&(*)(int&&, std::string&&);
typedef int&&(*TypedefFuncPtrReturnRvalue)(int&&);
typedef std::vector<int>&&(*TypedefFuncPtrReturnRvalueTemplate)(std::string&&);

// misc function patterns

void func13(int*&& ptr);
void func14(int(&&arr)[10]);
template<typename T>
void func15(std::vector<T>&& vec);
template<typename T>
auto func16(T&& x) -> decltype(std::forward<T>(x));
template<typename T>
auto func17(T&& x) -> decltype(std::vector<T&&>{x});
void func18(int&& x) noexcept;
void func18Def(int&& x) noexcept {
    (void)std::move(x);
}
template<typename... T>
void func19(T&&... arg) {
    // other_func(std::forward<T>(arg)...);
}
void func20(std::string&& s = std::string("default"));
void func21(int& x);
void func21(int&& x);

// static/virtual

class StaticClass {
static void staticMethod(int&& x);
};

class VirtualClass {
public:
virtual void virtualMethod(int&& x);
};

class DerivedClass : public VirtualClass {
void virtualMethod(int&& x) override;
};

// template return types

std::vector<int>&& func22();
std::map<std::string, std::vector<int>>&& func23();
MyClass&& func24();
template<typename T>
T&& func25();
auto func26() -> std::vector<int>&&;
auto func27() -> std::map<int, std::string>&&;
const std::vector<int>&& func28();
std::pair<int, std::string>&& func29();

class ReturnClass {
public:
std::vector<int>&& getVector();
std::map<int, int>&& getMap();
};

class StaticReturnClass {
static std::vector<std::string>&& getStrings();
};

std::function<void(std::tuple<int&&, std::tuple<int&&, std::vector<std::string&&>>>&&)> func30();

// multi-line bool

bool multiLineResult1 = condition1
    && condition2
    && condition3;

bool multiLineResult2 = (a > b)
    && (c < d)
    && (e == f);

bool multiLineResult3 = condition1
    && condition2
    || condition3
    && condition4;

void testMultiLine() {
    if (longCondition1
        && longCondition2
        && longCondition3)
    {
        int&&temp = getValue();
    }
}

bool checkAll() {
    return isValid()
        && isReady()
        && isEnabled();
}

// fold expressions

template<typename... Ts>
constexpr bool allTrueUnary(Ts... args) {
    return (... && args);
}

template<typename... Ts>
constexpr bool allTrueBinary(Ts... args) {
    return (true && ... && args);
}

template<typename... Ts>
constexpr bool allTrueRight(Ts... args) {
    return (args && ...);
}

template<typename... Ts>
constexpr bool foldWithRvalue(Ts&&... args) {
    return (... && static_cast<bool>(args));
}

template<typename... Ts>
constexpr auto foldMixed(Ts&&... args) {
    return (std::forward<Ts>(args) && ...);
}

// sfinae, noexcept

template<typename T,
    typename = std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value>>
void sfinaeFunc(T&& t);

template<typename T,
    typename = std::enable_if_t<
        std::is_move_constructible<T>::value
        && std::is_move_assignable<T>::value>>
T sfinaeReturn(T&& val) {
    return std::move(val);
}

template<typename T, typename U,
    std::enable_if_t<std::is_convertible<T, U>::value && std::is_class<T>::value, int> = 0>
void multiSfinae(T&& t, U&& u);

template<typename T>
void noexceptAnd(T&& t) noexcept(std::is_nothrow_move_constructible_v<T> && true);

template<typename T>
void noexceptComplex(T&& t) noexcept(
    std::is_nothrow_move_constructible_v<T>
    && std::is_nothrow_move_assignable_v<T>);

template<typename T, typename U>
void noexceptMulti(T&& t, U&& u) noexcept(
    std::is_nothrow_move_constructible_v<T>
    && std::is_nothrow_move_constructible_v<U>
    && noexcept(std::declval<T>() + std::declval<U>()));

class NoexceptClass {
    NoexceptClass(NoexceptClass&& other) noexcept(
        std::is_nothrow_move_constructible_v<int> && true);
};

template<typename T>
class ExplicitSfinaeClass {
    template<typename U = T,
        std::enable_if_t<sizeof(U) > 8 && std::is_trivially_copyable<U>::value, int> = 0>
    explicit ExplicitSfinaeClass(U&& val);

    template<typename U = T,
        std::enable_if_t<std::is_integral<U>::value && !std::is_same<U, bool>::value, int> = 0>
    operator U&&() &&;
};

template<typename T, typename U>
struct ExplicitMulti {
    template<typename V = T, typename W = U,
        std::enable_if_t<std::is_convertible<V, W>::value
            && sizeof(V) <= sizeof(W)
            && std::is_trivial<V>::value, int> = 0>
    ExplicitMulti(V&& t, W&& u);
};

// decltype, conversions

class ConversionClass {
    operator int&&() &&;
    operator int() const &&;
    operator std::string&&() &&;
    operator const std::string&() const &&;
    template<typename T>
    operator T&&() &&;
    explicit operator bool() const &&;
};

class TemplateConversion {
    template<typename T>
    operator std::vector<T>&&() &&;
    operator std::map<int, int>&&() &&;
};

decltype(true && false) decl1;
decltype(a && b) decl2;
decltype(getValue())&& decl3 = getValue();
decltype(expr)&& decl4 = std::move(expr);
template<typename T>
decltype(std::declval<T>() && std::declval<T>()) declBoolInside(T&& t);
template<typename T>
decltype(std::declval<T>())&& declRefOutside(T&& t);
decltype(auto) declAuto1 = std::move(value);
decltype(decltype(a && b)() && decltype(c && d)()) nestedDecl;

template<typename T>
decltype(static_cast<T&&>(std::declval<T>()).getValue()) getValueType(T&& t);
template<typename T>
decltype(helper<T&&>(t)) helperResult(T&& t);
template<typename T>
using NestedDecltype = decltype(std::forward<T&&>(std::declval<T>()));
template<typename T>
using DecltypeRvalueRef = decltype(T&&);

using MfpRvalue = void (MyClass::*)() &&;
using MfpConstRvalue = void (MyClass::*)() const &&;
using MfpParamRvalue = void (MyClass::*)(int&&) &&;
typedef void (MyClass::*MfpTypedef)() &&;
typedef int&& (MyClass::*MfpReturnRvalue)() &&;
using ComplexMFP = std::vector<int>&& (MyClass::*)(std::string&&) const &&;
using MFPArray = void (MyClass::*[10])() &&;

// static_assert, deleted/defaulted

static_assert(std::is_integral_v<int> && std::is_signed_v<int>);
static_assert(sizeof(int) == 4 && alignof(int) == 4, "Size check");

static_assert(
    std::is_move_constructible_v<std::string>
    && std::is_move_assignable_v<std::string>
    && std::is_nothrow_move_constructible_v<std::string>,
    "String must be efficiently movable");

template<typename T>
struct StaticAssertClass {
    static_assert(std::is_class_v<T> && !std::is_final_v<T>);
    void method(T&& t) {
        static_assert(std::is_rvalue_reference_v<T&&> && true);
    }
};

class DeletedRefQualified {
    void method() && = delete;
    void constMethod() const && = delete;
    void paramMethod(int&&) && = delete;
};

class DefaultedMoves {
    DefaultedMoves(DefaultedMoves&&) = default;
    DefaultedMoves& operator=(DefaultedMoves&&) = default;
    DefaultedMoves& operator=(const DefaultedMoves&) && = default;
};

class NoexceptDefaulted {
    NoexceptDefaulted(NoexceptDefaulted&&) noexcept = default;
    NoexceptDefaulted& operator=(NoexceptDefaulted&&) noexcept = default;
};

// template template params

template<template<typename> class C, typename T>
void processTemplate(C<T>&& container);

template<template<typename...> class C, typename... Ts>
C<Ts...>&& forwardContainer(C<Ts...>&& c);

template<template<typename> class C, typename T,
    std::enable_if_t<std::is_move_constructible<C<T>>::value && std::is_default_constructible<C<T>>::value, int> = 0>
void constrainedTemplate(C<T>&& container);

template<template<template<typename> class> class Outer, template<typename> class Inner>
void nestedTemplateTemplate(Outer<Inner>&& arg);

std::conditional_t<std::is_integral_v<int> && std::is_signed_v<int>, int&&, float&> condMember1;
std::conditional_t<A && B, std::conditional_t<C && D, T&&, U&&>, V&> condMember2;
std::enable_if_t<std::is_class_v<T> && std::is_move_constructible_v<T>, T&&> enableIfRvalue;

template<typename T>
auto nestedConditional(T&& t)
    -> std::conditional_t<std::is_rvalue_reference_v<T&&> && true, T&&, T&>;

// complex lambdas

auto lambdaBool1 = [](int&& t) {
    return std::is_integral<decltype(t)>::value && true;
};

auto lambdaBool2 = [](auto&& t) {
    return std::is_class<std::decay_t<decltype(t)>>::value
        && std::is_move_constructible<std::decay_t<decltype(t)>>::value;
};

auto lambdaGeneric = [](auto&& t) {
    if (sizeof(t) > 0 && true) {
        return std::forward<decltype(t)>(t);
    }
    return t;
};

auto lambdaMulti = [](auto&& t, auto&& u) {
    return (sizeof(t) > 0 && sizeof(u) > 0);
};

auto iifeResult1 = [](int&& x) { return x > 0; }(42) && flag;
auto iifeResult2 = [](int&& x) { return x; }(std::move(val)) && condition;

auto iifeChain = [](){ return true; }() && [](){ return true; }() && finalCheck;

auto&& iifeRvalue = [](int&& x) -> int&& { return std::move(x); }(42);

if ([](int&& x){ return x > 0; }(getValue()) && otherCondition) {
    int&& temp = getValue();
}

auto iifeCapture = [&](int&& x) { return x && captured; }(42) && external;

// mixed contexts

bool b1 = (x && y) ? getValue<int&&>() : getValue<float&&>();
void mixedDefault(int&& x = (a && b) ? 1 : 2);

auto multiContext = [](int&& x) { return x && true; };

auto ternaryMixed = (a && b) ? std::move(c) : (d && e) ? std::move(f) : std::move(g);

auto returnMixed(int&& x) -> decltype(x && true) {
    return x && condition;
}

// attributes, constexpr

[[nodiscard]] int&& attrReturn();
[[maybe_unused]] int&& attrVar = getValue();
void attrParam([[maybe_unused]] int&& x);

constexpr int&& constexprReturn();
constexpr bool constexprAnd = true && false;
constexpr auto constexprMixed(int&& x) -> bool { return x && true; }

// structured bindings, ctad

auto&& [xBind, yBind, zBind] = getTuple();

template<typename T>
struct Generator {
    struct promise_type {
        T&& currentValue;
        auto yield_value(T&& value) {
            currentValue = std::move(value);
        }
    };
};

template<typename T>
void ifConstexprTest(T&& t) {
    if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        int&& temp = static_cast<int&&>(t);
    }

    if constexpr (std::is_class_v<T>
        && std::is_move_constructible_v<T>
        && std::is_copy_constructible_v<T>)
    {
        T&& moved = std::move(t);
    }
}

class ComparisonClass {
    bool operator<(const ComparisonClass&) const && ;
    bool operator==(const ComparisonClass&) const &&;
};

template<typename T>
struct DeducedClass {
    DeducedClass(T&&);
};

template<typename T>
DeducedClass(T&&) -> DeducedClass<T>;

template<typename T>
void genericTemplate(T&& x);

template<typename T>
auto genericReturn(T&& x) -> decltype(x && true);

// sparen, variadics

template<typename... Ts>
void packTest(Ts&&... args) {
    bool all = (... && static_cast<bool>(args));
}

template<typename... Ts>
bool packAllTrue(Ts&&... args) {
    return (true && ... && args);
}

template<typename... Ts>
auto packForward(Ts&&... args) {
    return (std::forward<Ts>(args) && ...);
}

int a = 0;
bool isFoo = true;

if (a == Foo::a && isFoo)
{}
if (a == Foo::a && Foo::check())
{}
if (ns1::ns2::check() && ns3::validate())
{}
while (Foo::check() && Bar::isReady())
{}
for (int i = 0; Foo::check() && i < 10; ++i)
{}
if (a == Foo::a && a == Foo::a)
{}
auto val1 = (a == Foo::a && isFoo);
bool val2 = Foo::a && Foo::check();
auto val3 = ns1::ns2::check() && ns3::validate();

// init-if, init-switch, range-for

if (std::vector<int>&& v = getVector(); !v.empty()) {}
switch (std::vector<int>&& v = getVector(); v.size()) {
    case 0: break;
    default: break;
}
for (std::vector<int>&& item : getVectors()) {}
if (std::is_integral_v<int> && std::is_signed_v<int>) {}

switch (std::optional<int>&& opt = getOpt(); opt.has_value()) {
    case true: break;
    case false: break;
}
switch (std::map<std::string&&, int&&>&& m = getMap(); m.size()) {
    case 0: break;
    default: break;
}
switch (std::pair<int&&, std::string&&>&& p = getPair(); p.first) {
    case 0: break;
    default: break;
}
switch (std::is_integral_v<T&&> && std::is_signed_v<T&&>) {
    case true: break;
    case false: break;
}
switch (std::vector<int&&>&& v = getVec(); v.size() > 0 && v.front() > 0) {
    case true: break;
    case false: break;
}
switch (std::vector<std::pair<int&&, std::string&&>&&>&& vp = getVecPairs(); vp.size()) {
    case 0: break;
    default: break;
}

// operator overloads

class T {
public:
    T&& operator+(T&&) &&;
    T operator+(const T&) const &&;
    T&& operator+() &&;
    T&& operator-() &&;
    T&& operator~() &&;
    bool operator!() const &&;
    T&& operator++() &&;
    T operator++(int) &&;
    bool operator==(const T&) const &&;
    bool operator<(const T&) const &&;
    auto operator<=>(const T&) const && -> std::strong_ordering;
    bool operator&&(const T&) const &&;
    bool operator&&(T&&) &&;
    bool operator||(T&&) &&;
    T&& operator&(T&&) &&;
    T&& operator<<(T&&) &&;
    T&& operator=(T&&) &&;
    T&& operator+=(T&&) &&;
    T& operator=(const T&) &;
    T&& operator*() &&;
    T* operator->() &&;
    T&& operator->*(T&&) &&;
    T&& operator[](int) &&;
    T&& operator[](T&&) &&;
    T&& operator()() &&;
    T&& operator()(T&&) &&;
};

class OpMod {
    T&& operator+(T&&) && noexcept;
    T&& operator-(T&&) && noexcept(true);
    [[nodiscard]] T&& operator/(T&&) &&;
    auto operator%(T&&) && -> T&&;
    auto operator^(T&&) && -> decltype(auto);
    std::vector<int>&& operator|(std::vector<int>&&) &&;
    T& operator~() &;
    T&& operator~() &&;
    const T& operator~() const &;
    const T&& operator~() const &&;
    T& operator=(T&&) && = delete;
};

template<typename U>
class TplOp {
    TplOp<U>&& operator+(TplOp<U>&&) &&;
    U&& operator[](size_t) &&;
    template<typename... Args> U&& operator()(Args&&...) &&;
    operator U&&() &&;
    bool operator&&(const TplOp<U>&) const &&;
};

template<typename U>
class SfinaeOp {
    template<typename V = U, std::enable_if_t<std::is_integral_v<V> && std::is_signed_v<V>, int> = 0>
    SfinaeOp<U>&& operator+(SfinaeOp<U>&&) &&;
};

inline T&& T::operator+(T&& other) && { return std::move(*this); }
inline bool T::operator==(const T& other) const && { return true; }
inline auto T::operator%(T&& other) && -> T&& { return std::move(*this); }

class FriendOp {
    friend T&& operator+(T&&, T&&);
    friend bool operator&&(T&&, T&&);
};

T&& operator+(T&& lhs, T&& rhs);
std::ostream& operator<<(std::ostream& os, T&& obj);
T&& operator""_t(unsigned long long);

class MultiAmp {
    bool operator&&(MultiAmp&&) &&;
    bool operator&&(const MultiAmp&&) const &&;
    MultiAmp&& operator&(MultiAmp&&) &&;
    MultiAmp&& operator&&(MultiAmp&&) && noexcept;
};

// out-of-class definitions

class MyClass4;
template<typename T> class Container2;
namespace ns2 { class Inner2; }

MyClass4&& MyClass4::getValue() && { return std::move(*this); }
MyClass4&& MyClass4::process(MyClass4&& other) && { return std::move(other); }
template<typename T>
T&& Container2<T>::get() && { return std::move(m_value); }
template<typename T>
Container2<T&&>&& Container2<T&&>::move() && { return std::move(*this); }
ns2::Inner2&& ns2::Inner2::move() && { return std::move(*this); }
MyClass4&& MyClass4::moveNoexcept() && noexcept { return std::move(*this); }
MyClass4&& MyClass4::create(MyClass4&& src) { return std::move(src); }
MyClass4&& operator+(MyClass4&& lhs, MyClass4&& rhs) { return std::move(lhs); }
