static void func() {
    [[FOO alloc] target:true ? 1 : 2
                target2:ivar ?: fallback
                target3:ivar ?: fallback
                target4:1];
}
