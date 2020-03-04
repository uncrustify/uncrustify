int (^myBlock)(int) = ^(int num) {
    return num * multiplier;
};
// for comparison
int (*fcnptr)(int);

int d = i % 10;
repeat(10, ^{ putc('0' + d);
       });


void (^block)(void);
typedef void (^vstr_t)(char *);
typedef void (^workBlk_t)(void);

void AllLinesInFile(char *f, vstr_t block)
{
    FILE *fp = fopen(f, "r");

    if (!fp)
    {
        return;
    }
    char line[1024];

    while (fgets(line, 1024, fp))
    {
        block(line);
    }
    fclose(fp);
}


@implementation NSArray (WWDC)
-(NSArray *)map: (id (^)(id)) xform
{
    id result = [NSMutableArray array];

    for (id elem in self)
    {
        [result addObject: xform(elem)];
    }
    return result;
}

-(NSArray *)collect: (BOOL (^)(id)) predicate
{
    id result = [NSMutableArray array];

    for (id elem in self)
    {
        if (predicate(elem))
        {
            [result addObject: elem];
        }
    }
    return result;
}

// corner case: block literal in use with return type
id longLines = [allLines collect: ^BOOL (id item) {
                    return [item length] > 20;
                }];

// corner case: block literal in use with return type
id longLines = [allLines collect: ^BOOL *(id item) {
                    return [item length] > 20;
                }];

@end

// 1. block literal: ^{ ... };
// 2. block declaration: return_t (^name) (int arg1, int arg2, ...) NB: return_t is optional and name is also optional
// 3. block inline call ^ return_t (int arg) { ... }; NB: return_t is optional
