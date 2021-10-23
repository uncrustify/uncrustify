#ifndef __LIBR_BACKENDS_H
#define __LIBR_BACKENDS_H

char    szPath[512];
char *  pszFilePart;
int     ret;
double *pd;

INTERNAL_FN libr_intstatus add_section(libr_file *file_handle, char *resource_name, libr_section **retscn);
INTERNAL_FN void           *data_pointer(libr_section *scn, libr_data *data);
INTERNAL_FN size_t         data_size(libr_section *scn, libr_data *data);

libr_intstatus add_section(libr_file *file_handle, char *resource_name, libr_section **retscn);
void           *data_pointer(libr_section *scn, libr_data *data);
size_t         data_size(libr_section *scn, libr_data *data);

libr_intstatus add_section(libr_file *file_handle, char *resource_name, libr_section **retscn);
void           *data_pointer(libr_section *scn, libr_data *data);

void   *data_pointer(libr_section *scn, libr_data *data);
size_t data_size(libr_section *scn, libr_data *data);

libr_intstatus add_section(libr_file *file_handle, char *resource_name, libr_section **retscn);

void *data_pointer(libr_section *scn, libr_data *data);

size_t data_size(libr_section *scn, libr_data *data);

#endif /* __LIBR_BACKENDS_H */
