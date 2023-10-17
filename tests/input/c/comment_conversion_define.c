#define HDROFF_TYPE(b)      sizeof(ElfPreHeader)             /* ElfXX_Half  e_type; */

typedef struct _libr_file
{
  unsigned int  version;
} libr_file;

#define HDROFF_TYPE(b)      sizeof(    /* ElfXX_Half  e_type; */    \
		ElfPreHeader /* ElfXX_Half  e_type; */ \
		) /* Repeat ElfXX_Half  e_type; */

/* ElfXX_Half  e_shstrndx; */

#ifndef DOXYGEN_SHOULD_SKIP_THIS  /* TEST */

/* Do something */
void foo()
{
	/* comment */
	int i = 0;
}

#endif

