// Issue #4777: a prototype led by an attribute-like prefix must stay in the
// same group as its neighbours (no blank lines between them).

// [[...]] attributes, single and stacked
class A_cpp11 {
void Foo();
[[nodiscard]] int Bar();
[[deprecated("x")]] [[maybe_unused]] int Qux();
void Baz();
};

// __attribute__((...)), single, stacked and with nested parens
class B_gnu {
void Foo();
__attribute__((noinline)) int Bar();
__attribute__((visibility("default"))) __attribute__((a)) int Qux();
void Baz();
};

// __unused and alignas
class C_unused_alignas {
void Foo();
__unused int Bar();
alignas(16) int Qux();
_Alignas(16) int Quux();
void Baz();
};

// several different prefixes in a row, in any order
class D_mixed {
void Foo();
[[nodiscard]] __declspec(dllexport) int Bar();
__declspec(dllexport) [[nodiscard]] int Qux();
alignas(8) __attribute__((a)) int Quux();
__attribute__((a)) alignas(8) int Quuux();
void Baz();
};

// prefixed prototype first, last, and a run of them
class E_positions {
[[nodiscard]] int First();
void Middle();
[[nodiscard]] int Last();
};

class F_run {
[[nodiscard]] int Foo();
[[nodiscard]] int Bar();
[[nodiscard]] int Baz();
};

// constructors and destructors
class G_special {
G_special();
[[deprecated]] G_special(int);
~G_special();
};

// friend
class H_friend {
void Foo();
[[nodiscard]] friend int Bar();
friend __declspec(dllexport) int Qux();
void Baz();
};

// namespace and file scope
namespace N {
void Foo();
[[nodiscard]] int Bar();
__attribute__((noinline)) int Qux();
void Baz();
}

void FileFoo();
[[nodiscard]] int FileBar();
alignas(16) int FileQux();
void FileBaz();
