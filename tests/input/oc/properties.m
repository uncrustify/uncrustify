#define nonnull_strong nonnull, strong
#define myatomic nonatomic
@interface UCTestClass ()

@property (nonatomic, strong, null_unspecified, readonly) NSString* test1;
@property (strong, readonly, nonatomic, nullable) NSString* test2;
@property (strong, readonly, getter=test2Getter, nonatomic, nonnull) NSString* test3;
@property (strong, readonly, getter=test2Getter, nonatomic, setter=test2Setter:, null_resettable) NSString* test4;
@property (class, readonly, getter=test5Getter, nonatomic, nonnull, assign) NSString* test5;
@property (class, assign, getter=test5Getter, myatomic, nonnull_strong) NSString* test6;

@end
