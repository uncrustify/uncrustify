
static inline long
get_tv32(struct timeval *o, struct timeval32 __user *i)
{
   return(!access_ok(VERIFY_READ, i, sizeof(*i)) ||
          (__get_user(o->tv_sec, &i->tv_sec) |
           __get_user(o->tv_usec, &i->tv_usec)));
}

static inline long
get_tv32(struct timeval *o, struct timeval32 __user *i)
{
   return(!access_ok(VERIFY_READ, i, sizeof(*i)) ||
          (__get_user(o->tv_sec, &i->tv_sec) |
           __get_user(o->tv_usec, &i->tv_usec)));
}

const char *
dcrp_license_feature(int32_t idx)
{
#define FEATURESTR(f)      \
case DCRMIB_LICENSE_ ## f: \
   return(DCRP_LICENSE_FEATURE_ ## f ## _STR)

   switch (idx)
   {
      DCRP_LICENSE_FOREACH_FEATURES(FEATURESTR);
   }

   return("");
}


static int
isValidLicenseType(int32_t idx)
{
#define CHECKFEATURE(f)    \
case DCRMIB_LICENSE_ ## f: \
   return(1)

   switch (idx)
   {
      DCRP_LICENSE_FOREACH_FEATURES(CHECKFEATURE);
   }

   return(n * foo(5));
}
