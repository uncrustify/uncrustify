extern /*@observer@*/ /*@null@*/ const dcroid_t *dcrp_oidget
(
   /*@in@*/ const char *h,
   /*@in@*/ const char *t
) /*@ensures maxRead(result) >= 0@*/;

extern /*@observer@*/ const char *dcrp_oidlabel
(
   /*@in@*/ const dcroid_t *oid
);
