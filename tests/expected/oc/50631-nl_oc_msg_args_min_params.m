static void function() {
    [object param1:nil
            param2:nil
            param3:nil
            param4:nil];

    [object param1:nil param2:nil param3:nil];

    [object param1:nil param2:nil];

    [object param1:nil];

    [object func];

    [obj param1:nil param2:nil param3:[obj2 param1:nil param2:nil]];

    [obj param1:nil
         param2:[obj2 param1:nil
                      param2:nil
                      param3:nil
                      param4:nil
                      param5:nil
                      param6:nil]
         param3:nil];

    [obj param1:nil param2:[obj2 param1:nil
                                 param2:nil
                                 param3:nil
                                 param4:nil
                                 param5:nil
                                 param6:nil] param3:nil];
}
