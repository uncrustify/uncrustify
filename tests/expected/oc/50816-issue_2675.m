@interface Example1 : NSObject
typedef ObjectType0 X;
typedef ObjectType1 _Nullable (^Handler1)(id<Fragment> fragment);
typedef ObjectType2 _Nullable (^Handler2)(id<Fragment> fragment);
@end

@interface Example2 : NSObject
typedef ObjectType1 _Nullable (^Handler1)(id<Fragment> fragment);
typedef ObjectType2 _Nullable (^Handler2)(id<Fragment> fragment);
@end

@interface AnotherExample1 : NSObject
SOME_MACRO_OPEN
				- (instancetype)init;

SOME_MACRO_CLOSE
@end

SOME_MACRO_OPEN
				@interface AnotherExample2 : NSObject
SOME_MACRO_CLOSE
- (instancetype)init;

@end

@interface SomeInterface : NSObject

// Some comment goes here
@end

@interface YetAnotherExample : NSObject

// What about this comment
// here
- (instancetype)init;
@end

@interface YetOneAnotherExample : NSObject

/// What about this comment
/// here
- (instancetype)init;
@end

@interface YetOneOtherExample : NSObject

/// What about this comment
/// here
- (instancetype)init;
@end


@interface YetOneMoreExample : NSObject

/* Different comment pattern */
- (instancetype)init;
@end


@interface YetOneMoreOtherExample : NSObject

/* Multiline
   Comments
 */
- (instancetype)init;
@end
