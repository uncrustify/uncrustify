BEGIN {
  count = 0;
}
{
  line = $0;
  comma = index(line, ",");
  #printf("%d, The line=%s\n", comma, line);
  if (comma > 0) {
    UO_part = substr(line, 4, comma - 4);
    #printf("uo=%s\n", UO_part);
    slash = index(UO_part, "//");
    if (slash > 0) {
      #printf("%d, UO_part=%s\n", slash, UO_part);
    } else {
      option_count = index(UO_part, "UO_option_count");
      if (option_count == 0)
      {
        count = count + 1;
      }
    }
  }
}
END {
  printf("there are %d options\n", count);
}
