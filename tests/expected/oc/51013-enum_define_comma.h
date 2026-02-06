// Test: #define inside enum should not get trailing comma added
typedef NS_CLOSED_ENUM(NSUInteger, TestEnum) {
	TestEnumValueA,
	TestEnumValueB,
	TestEnumValueC,

#define TestEnumLastValue TestEnumValueC
};
