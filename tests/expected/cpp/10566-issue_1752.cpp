#define WARN_IF(EXP) \
	do { if (EXP) \
	     fprintf (stderr, "Warning: " #EXP "\n"); } \
