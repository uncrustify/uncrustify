
const char *token_names[] =
{
   [CT_POUND] = "POUND",
   [CT_PREPROC] = "PREPROC",
   [CT_PREPROC_BODY] = "PREPROC_BODY",
   [CT_PP] = "PP",
   [CT_ELIPSIS]  = "ELIPSIS",
   [CT_NAMESPACE]= "NAMESPACE",
   [CT_NEW]  = "NEW",
   [CT_OPERATOR] = "OPERATOR",
   [CT_THROW]  = "THROW",
   [CT_TRY]   = "TRY",
   [CT_USING]   = "USING",
   [CT_PAREN_OPEN] = "PAREN_OPEN",
};


int main(int argc, char *argv[])
{
   struct junk a[] = {
      { "version", 0, 0, 0},
      {"file", 1, 150, 'f'},
      {"config", 1, 0, 'c'},
   {"parsed", 25, 0, 'p'},
      { NULL, 0, 0, 0}
   };
}

 color_t colors[] = {
    {"red",{255,0,0}},{"blue",{0,255,0}},
    {"green",{0,0,255}},{"purple",{255,255,0}},
 };

 struct foo_t bar = {
    .name = "bar",
 .age  = 21
};



struct foo_t bars[] = {
    [0] = { .name = "bar",
        .age    = 21 },
   [1] = { .name = "barley",
         .age = 55 },
};

