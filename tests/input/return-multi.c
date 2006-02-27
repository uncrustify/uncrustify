
static inline long
get_tv32(struct timeval *o, struct timeval32 __user *i)
{
	return !access_ok(VERIFY_READ, i, sizeof(*i)) ||
                (__get_user(o->tv_sec, &i->tv_sec) |
		 __get_user(o->tv_usec, &i->tv_usec));
}

static inline long
get_tv32(struct timeval *o, struct timeval32 __user *i)
{
	return (!access_ok(VERIFY_READ, i, sizeof(*i)) ||
                (__get_user(o->tv_sec, &i->tv_sec) |
		 __get_user(o->tv_usec, &i->tv_usec)));
}

