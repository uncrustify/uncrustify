
/* Assuming a 60-column limit */
static int short_function_name(struct device *dev,
                               struct device_driver *drv)
{
    call_some_really_long_function.of_some_sort(
        some_long_parameter1, some_long_parameter2);

    abc = call_some_other_really_long_function.of_some_sort(
              some_long_parameter1, some_long_parameter2);

    abc.def.ghi = call_some_other_really_long_function.
                  of_some_sort(some_long_parameter1,
                               some_long_parameter2);

    return 1;
}

static void some_really_long_function_name(
    struct device *dev, struct device_driver *drv)
{
    if ((some_variable_name &&
         somefunction(param1, param2, param3))) {
        asdfghjk = asdfasdfasd.aasdfasd +
                   (asdfasd.asdas * 1234.65);
    }


    register_clcmd( "examine", "do_examine", -1,
                   "-Allows a player to examine the health and armor of a teammate" );
    register_clcmd( "/examine", "do_examine", -1,
                   "-Allows a player to examine the health and armor of a teammate" );
}

