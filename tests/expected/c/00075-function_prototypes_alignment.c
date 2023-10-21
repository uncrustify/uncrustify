libr_intstatus set_data1(libr_file *file_handle, libr_section *scn, libr_data *data, off_t offset,
        char *buffer, size_t size);
void           write_output1(libr_file *file_handle);
libr_intstatus open_handles1(libr_file *file_handle, char *filename, libr_access_t access);

libr_intstatus set_data2(libr_file *file_handle, libr_section *scn, libr_data *data, off_t offset,
        char *buffer, size_t size);
libr_intstatus open_handles2(libr_file *file_handle, char *filename, libr_access_t access);
void           write_output2(libr_file *file_handle);

INTERNAL_FN libr_intstatus set_data1(libr_file *file_handle, libr_section *scn, libr_data *data,
        off_t offset, char *buffer, size_t size);
INTERNAL_FN void           write_output1(libr_file *file_handle);
INTERNAL_FN libr_intstatus open_handles1(libr_file *file_handle, char *filename,
        libr_access_t access);

INTERNAL_FN libr_intstatus set_data2(libr_file *file_handle, libr_section *scn, libr_data *data,
        off_t offset, char *buffer, size_t size);
INTERNAL_FN libr_intstatus open_handles2(libr_file *file_handle, char *filename,
        libr_access_t access);
INTERNAL_FN void           write_output2(libr_file *file_handle);
