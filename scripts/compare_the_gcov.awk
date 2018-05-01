BEGIN {
  number_of_lines    = 0;
  number_of_header   = 0;
  T_number_of_lines  = 0;
  T_number_of_header = 0;
  input_file  = in_file;
  output_file = out_file;
  #
  # get the first line, if any.
  getline aaa <input_file;
  if (ERRNO != "") {
    #printf("ERRNO is %s\n", ERRNO);
    #printf("a new file will be created at %s\n", output_file);
    # totals-file not found, this is the first run.
    first_run = "yes";
  } else {
    # totals-file is found. Read it into the arrays
    first_run = "no";
    for ( i = 1; i < 20000; i++) {
      theLine = aaa;
      where_is_colon_1 = index(theLine, ":");
      part_1 = substr(theLine, 1, where_is_colon_1 - 1);
      rest_1 = substr(theLine, where_is_colon_1 + 1);
      where_is_colon_2 = index(rest_1, ":");
      part_2 = substr(rest_1, 1, where_is_colon_2 - 1) + 0;
      rest_2 = substr(rest_1, where_is_colon_2 + 1);
      if (part_2 == 0) {
        # header part
        T_number_of_header = T_number_of_header + 1;
        T_header_part1[T_number_of_header] = part_1;
        T_header_part2[T_number_of_header] = part_2;
        T_header_part3[T_number_of_header] = rest_2;
      } else {
        # source lines
        # a new line
        T_number_of_lines = part_2;
        T_source_part1[T_number_of_lines] = part_1;
        T_source_part2[T_number_of_lines] = part_2;
        T_source_part3[T_number_of_lines] = rest_2;
      }

      aaa = "";
      # get the next line
      getline aaa <input_file;
      if (aaa == "") {
        # EOF
        break;
      }
    }
    close(input_file);
    # Test it
    #printf("Test it\n");
    #for (i = 1; i <= T_number_of_header; i++) {
    #  printf("%8s:%5d:%s\n", T_header_part1[i], T_header_part2[i], T_header_part3[i]);
    #}
    #for (i = 1; i <= T_number_of_lines; i++) {
    #  printf("%8s:%5d:%s\n", T_source_part1[i], T_source_part2[i], T_source_part3[i]);
    #}
  }
}

{
  theLine = $0;
  where_is_colon_1 = index(theLine, ":");
  part_1 = substr(theLine, 1, where_is_colon_1 - 1);
  rest_1 = substr(theLine, where_is_colon_1 + 1);
  where_is_colon_2 = index(rest_1, ":");
  part_2 = substr(rest_1, 1, where_is_colon_2 - 1) + 0;
  rest_2 = substr(rest_1, where_is_colon_2 + 1);
  if (part_2 == 0) {
    # header part
    number_of_header = number_of_header + 1;
    header_part1[number_of_header] = part_1;
    header_part2[number_of_header] = part_2;
    header_part3[number_of_header] = rest_2;
  } else {
    # source lines
    # a new line
    number_of_lines = part_2;
    source_part1[number_of_lines] = part_1;
    source_part2[number_of_lines] = part_2;
    source_part3[number_of_lines] = rest_2;
    where_ = index(part_1, "-");
    if (where_ > 0) {
      # don't take care
    } else {
      where_2 = index(part_1, "#####");
      if (where_2 > 0) {
        # don't take care
      } else {
        d_part_1 = part_1 + 0;
        # look at T_source_part1[part_2]
        where_3 = index(T_source_part1[part_2], "#####");
        if (where_3 > 0) {
          sum = d_part_1;
          # write the sum to T_source_part1
          T_source_part1[part_2] = d_part_1; 
        } else {
          d_T = T_source_part1[part_2] + 0;
          sum = d_part_1 + d_T;
          # write the sum back to T_source_part1
          T_source_part1[part_2] = sum; 
        }
      }
    }
  }
}
END {
  if (first_run == "yes") {
    # copy to T_
    T_number_of_header = number_of_header;
    T_number_of_lines  = number_of_lines;
    for(i = 1; i <= T_number_of_header; i++) {
      T_header_part1[i] = header_part1[i]; 
      T_header_part2[i] = header_part2[i]; 
      T_header_part3[i] = header_part3[i]; 
    }
    for (i = 1; i <= T_number_of_lines; i++) {
      T_source_part1[i] = source_part1[i]; 
      T_source_part2[i] = source_part2[i]; 
      T_source_part3[i] = source_part3[i]; 
    }
  }
  #printf("T_number_of_header is %d\n", T_number_of_header);
  #printf("T_number_of_lines  is %d\n", T_number_of_lines);

  # delete the previous version
  printf("") > output_file;
  for(i = 1; i <= T_number_of_header; i++) {
    printf("%9s:%5d:%s\n", T_header_part1[i], T_header_part2[i], T_header_part3[i]) >> output_file;
  }
  for (i = 1; i <= T_number_of_lines; i++) {
    printf("%9s:%5d:%s\n", T_source_part1[i], T_source_part2[i], T_source_part3[i]) >> output_file;
  }
  close(output_file);
}
