int *(^  blkReturningPointer)(int) = ^ int *(int a) {
    return a + 1;
};

void (^ blk2)(int *) = ^ (int *b) {
    *b = 1;
};


int (^oneFrom)(int) = ^  (int anInt) {
    return anInt - 1;
};

// this should not be flagged as OC_BLOCK_CARET
int x = 12 ^ 23;
