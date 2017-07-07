@interface UCTestClass ()

@property (nonatomic, strong, null_unspecified, readonly) NSString* test1;
@property (strong, readonly, nonatomic, nullable) NSString* test2;
@property (strong, readonly, getter=test2Getter, nonatomic, nonnull) NSString* test3;
@property (strong, readonly, getter=test2Getter, nonatomic, setter=test2Setter:, null_resettable) NSString* test4;

@end
