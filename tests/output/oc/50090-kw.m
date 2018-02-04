#include <stdio.h>
#include <objc/Object.h>

@protocol Job
-do;
@end

@interface MyJob : Object<Job>
-do;
@end
@implementation MyJob
-do
{
   printf("Doing Job\n");
   return self;
}
@end

@interface JobExecutor : Object
-doWith: (id<Job>)job for: (int)count;
@end

@implementation JobExecutor
-doWith: (id<Job>)job for: (int)count
{
   for (int i = 0; i < count; ++i)
   {
      [job do];
   }
   return self;
}
@end
