class X14
{
public:
X14();
~X14() = default;
X14(const X14& rhs) = default;
X14& operator=(const X14& rhs) = default;
X14(X14&& rhs) = delete;
X14& operator=(X14&& rhs) = delete;
};
