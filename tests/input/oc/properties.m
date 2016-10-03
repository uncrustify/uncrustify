@interface UCTestClass ()

@property (nonatomic, strong, nonnull, readonly) NSString* test1;
@property (strong, readonly, nonatomic, nonnull) NSString* test2;
@property (strong, readonly, getter=test2Getter, nonatomic, nonnull) NSString* test3;
@property (strong, readonly, getter=test2Getter, nonatomic, setter=test2Setter, nonnull) NSString* test4;

@end
