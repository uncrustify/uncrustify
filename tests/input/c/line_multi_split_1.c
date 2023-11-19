static int buf_write_do_autocmds(buf_T *buf, char **fnamep, char **sfnamep, char **ffnamep,
                                 linenr_T start, linenr_T *endp, exarg_T *eap, bool append,
                                 bool filtering, bool reset_changed, bool overwriting, bool whole,
                                 const pos_T orig_start, const pos_T orig_end)
{
}

static int buf_write_make_backup(char *fname, bool append, FileInfo *file_info_old, vim_acl_T acl,
                                 int perm, unsigned bkc, bool file_readonly, bool forceit,
                                 int *backup_copyp, char **backupp, Error_T *err)
{
}

Channel *channel_job_start(char **argv, const char *exepath, CallbackReader on_stdout,
                           CallbackReader on_stderr, Callback on_exit, bool pty, bool rpc,
                           bool overlapped, bool detach, ChannelStdinMode stdin_mode,
                           const char *cwd, uint16_t pty_width, uint16_t pty_height, dict_T *env,
                           varnumber_T *status_out)
{
}

void vim_str2nr(const char *const start, int *const prep, int *const len, const int what,
                varnumber_T *const nptr, uvarnumber_T *const unptr, const int maxlen,
                const bool strict, bool *const overflow)
{
}

static inline int json_decoder_pop(ValuesStackItem obj, ValuesStack *const stack,
                                   ContainerStack *const container_stack, const char **const pp,
                                   bool *const next_map_special, bool *const didcomma,
                                   bool *const didcolon)
{
}

void extmark_set(buf_T *buf, uint32_t ns_id, uint32_t *idp, int row, colnr_T col, int end_row,
                 colnr_T end_col, Decoration *decor, bool right_gravity, bool end_right_gravity,
                 bool no_undo, bool invalidate, Error *err)
{
}
