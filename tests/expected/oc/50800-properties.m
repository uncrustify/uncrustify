#define nonnull_strong nonnull, strong
#define myatomic nonatomic
@interface UCTestClass ()

@property (nonatomic,readonly,strong,null_unspecified) NSString* test1;
@property (nonatomic,readonly,strong,nullable) NSString* test2;
@property (nonatomic,readonly,strong,nonnull,getter=test2Getter) NSString* test3;
@property (nonatomic,readonly,strong,null_resettable,getter=test2Getter,setter=test2Setter:) NSString* test4;
@property (class,nonatomic,readonly,assign,nonnull,getter=test5Getter) NSString* test5;
@property (class,assign,getter=test5Getter,myatomic,nonnull_strong) NSString* test6;

@end
