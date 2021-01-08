int main()
{
  int sa_family;
  int d;
  int scopeid;
  switch (sa_family)
    {
    case 1:
#ifdef AF_INET6
      if (d == 1)
	scopeid = 1;
      else
	scopeid = 2;
      return 5;
#else
      return 6;
#endif

    case 2:
#ifdef AF_INET6
      TQString scopeid("%");
      if (d->addr.generic->sa_family == AF_INET6 && d->addr.in6->sin6_scope_id)
	scopeid += TQString::number(d->addr.in6->sin6_scope_id);
      else
	scopeid.truncate(0);
      return d->ref.ipAddress().toString() + scopeid;
#endif

    case 3:
#ifdef AF_INET6
      TQString scopeid("%");
      if (d->addr.generic->sa_family == AF_INET6 && d->addr.in6->sin6_scope_id)
	scopeid += TQString::number(d->addr.in6->sin6_scope_id);
      else
	scopeid.truncate(0);
      return d->ref.ipAddress().toString() + scopeid;
#endif
    }
}

