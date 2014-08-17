typedef boost::shared_ptr < RatherLongClassName > sp_RatherLongClassName_t;
int main()
{
    int argument = 1;
    sp_RatherLongClassName_t ratherLongVariableName1(new RatherLongClassName(argument,
    argument, argument));

    int the_result = a_very_long_function_name_taking_most_of_the_line(argument,
    argument, argument);
    return 0;
}
