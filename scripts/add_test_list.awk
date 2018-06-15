BEGIN {
  Test_Count = 0;
  count_cpp = split(sources_cpp, source_list_cpp, " ");
  printf("#count_cpp= %d\n", count_cpp);
  count_h = split(sources_h, source_list_h, " ");
  printf("#count_h= %d\n", count_h);
}
{
  theLine = $0;
  command = substr(theLine, 10);
  split(command, parts, " ");
  number = parts[1];
  lang = substr(parts[4], 14);
  l_lang = length(lang);
  lang_2 = substr(lang, 1, l_lang - 1);
  config = substr(parts[5], 16);
  input_file = substr(parts[7], 15);

  printf("echo \"Run uncrustify: The TESTNUMBER is %s\"\n", number);
  printf("rm -rf %s\n", number);
  printf("mkdir %s\n", number);
  printf("cd %s\n", number);
  printf("mkdir save\n");
  printf("../uncrustify -q -c \"../../tests/%s -f \"../../tests/%s -l %s -o /dev/null\n",
         config, input_file, lang_2);
  for (i = 1; i <= count_cpp; i++) {
    source_file = source_list_cpp[i];
    function_file = sprintf("../CMakeFiles/uncrustify.dir/src/%s.gcno", source_file);
    printf("if [ -s %s ] ;\n", function_file);
    printf("then\n");
    printf("  gcov %s 2> /dev/null 1> /dev/null\n", function_file, source_file);
    printf("fi\n");
    printf("if [ -s %s.* ] ;\n", source_file);
    printf("then\n");
    printf("   mv -f %s.* ./save/\n", source_file);
    printf("fi\n");
  }
  for (i = 1; i <= count_h; i++) {
    source_file = source_list_h[i];
    printf("if [ -s %s.* ] ;\n", source_file);
    printf("then\n");
    printf("   mv -f %s.* ./save/\n", source_file);
    printf("fi\n");
  }
  printf("  rm *.gcov\n");
  printf("  mv save/* .\n");
  printf("rmdir save\n");
  printf("cd ..\n\n");
  # to brake before the end
  #Test_Count = Test_Count + 1;
  #if ( Test_Count == 1000) {
  #if ( Test_Count == 109) {
  #if ( Test_Count == 2) {
  #  printf("exit\n");
  #}
}
