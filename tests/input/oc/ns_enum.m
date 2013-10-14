// The semicolons at the end of these declarations are not superfluous.
typedef NS_ENUM (NSUInteger, MyEnum) {MyValue1, MyValue2, MyValue3};
typedef NS_OPTIONS (NSUInteger, MyBitmask) {MyBit1, MyBit2, MyBit3};

// NS_ENUM specifies the type and name of the enum.
typedef enum {
MyValue1,
MyValue2,
MyValue3
} MyEnum;
typedef NS_ENUM (NSUInteger, MyEnum) {
MyValue1,
MyValue2,
MyValue3
};

// NS_OPTIONS is equivalent to NS_ENUM, but semantically used for bitmask enums.
typedef enum {
MyBit1 = (1u << 0),
MyBit2Longer = (1u << 1),
MyBit3ThatIsConsiderablyMoreVerbose = (1u << 2)
} MyBitmask;
typedef NS_OPTIONS (NSUInteger, MyBitmask) {
MyBit1 = (1u << 0),
MyBit2Longer = (1u << 1),
MyBit3ThatIsConsiderablyMoreVerbose = (1u << 2)
};
