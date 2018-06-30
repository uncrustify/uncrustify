#if UNITY_DEFER_GRAPHICS_JOBS_SCHEDULE
void GfxDevice::ScheduleAsyncJob(AsyncCommandJobFunc* jobFunc, GfxDeviceAsyncCommand* cmd, const JobFence& depends, JobBatchDispatcher& dispatcher)
#else
JobFence& GfxDevice::ScheduleAsyncJob(AsyncCommandJobFunc* jobFunc, GfxDeviceAsyncCommand* cmd, const JobFence& depends, JobBatchDispatcher& dispatcher)
#endif // #if UNITY_DEFER_GRAPHICS_JOBS_SCHEDULE
