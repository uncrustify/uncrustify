MOCK_METHOD(bool, isThat, (const std::string& name), (override));
MOCK_METHOD(bool, isThat, (std::string& name), (override));
MOCK_METHOD(bool, isThat, std::string& name, (override));
MOCK_METHOD(bool, isThat, const std::string& name, (override));
MyFunction(const std::string& some_value);
MOCK_METHOD(void, isThat, (Callback const&), (override));

class MyClass
{
constexpr explicit MyClass(MyClass*& a) noexcept;
};

SomeClass::SomeClass(unsigned& counter, MyOtherType& lock)
	: thisThing(counter)
	, thatThing(lock)
{
}

template<typename T = unsigned>
struct TemplatedClass
{
	TemplatedClass(T& thing) {
	}
};

MyThing* MyClass::my_function()
{
	return &_theirThing;
}

// Shouldn't change
EXPECT_EQ(static_cast<int>(MyEnumm::Last & MyEnumm::MyValue));
#define SOME_MACRO(type, sss) if(n & type) { if(!k.doofer()) { k += "| "; } k += sss; }
#define SOME_OTHER_MACRO(ttt, sss) (&ttt, &sss)
