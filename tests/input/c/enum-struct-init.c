
enum foo_idx
{
   FOO_1,
   FOO_2,
   FOO_3,
};

struct foo
{
   const char *str;
   int        values[8];
};

struct foo bar[] =
{
   [FOO_1] = {
      "junk",
      { 1 } },

   [FOO_2] = {
      "morejunk",
      {1, 2, 3} },

   [FOO_3] = {
      "somemore",
      {1, 2, 3, 4, 5, 6} },

};


