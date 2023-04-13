typedef int LIST_tzHEAD;
typedef int tucBOOL;
struct LIST_zNODE { int a; int b;};
struct LIST_zzzDATA { int a; int b;};

int foo1( LIST_tzHEAD *pList,
          tucBOOL (   *pFn )( struct LIST_zNODE   *pNode,
                              struct LIST_zzzDATA *pListData,
                              void                *arg1 ),
          void        *arg2 );
