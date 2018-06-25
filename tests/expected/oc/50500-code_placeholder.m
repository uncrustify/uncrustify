double          delayInSeconds = 2.0;
dispatch_time_t popTime        = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
   <#code to be executed on the main queue after delay#>
});
