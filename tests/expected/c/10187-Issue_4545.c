#define BAR 8
#define FOO 3

typedef struct my_struct my_struct_t;

struct my_struct
{
	uint8_t var[FOO * ((unsigned int)BAR)];
};

typedef struct example
{
	uint8_t var[FOO * ((unsigned int)BAR)];
} example_t;
