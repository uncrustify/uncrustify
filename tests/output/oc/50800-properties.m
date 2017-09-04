@interface UCTestClass ()

@property (nonatomic,readonly,strong,null_unspecified) NSString* test1;
@property (nonatomic,readonly,strong,nullable) NSString* test2;
@property (nonatomic,readonly,strong,nonnull,getter=test2Getter) NSString* test3;
@property (nonatomic,readonly,strong,null_resettable,getter=test2Getter,setter=test2Setter:) NSString* test4;

@end
