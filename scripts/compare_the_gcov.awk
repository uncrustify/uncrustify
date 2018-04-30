BEGIN {
  number_of_lines    = 0;
  number_of_header   = 0;
  T_number_of_lines  = 0;
  T_number_of_header = 0;
  input_file  = in_file;
  output_file = out_file;
  #printf("input_file is %s, output_file is %s\n", input_file, output_file);
  # get the first line, if any.
  getline aaa <input_file;
  if (ERRNO != "") {
    #printf("ERRNO is %s\n", ERRNO);
    #printf("a new file will be created at %s\n", output_file);
    # totals-file not found, this is the first run.
    first_run = "yes";
  } else {
    #printf("LESEN: input_file is %s\n", input_file);
    # totals-file is found. Read it into the arrays
    first_run = "no";
    for ( i = 1; i < 200; i++) {
      theLine = aaa;
      #printf("theLine is %s\n", theLine);
      where_is_colon_1 = index(theLine, ":");
      part_1 = substr(theLine, 1, where_is_colon_1 - 1);
      rest_1 = substr(theLine, where_is_colon_1 + 1);
      where_is_colon_2 = index(rest_1, ":");
      part_2 = substr(rest_1, 1, where_is_colon_2 - 1) + 0;
      rest_2 = substr(rest_1, where_is_colon_2 + 1);
      #printf("part_2 is %d\n", part_2);
      if (part_2 == 0) {
        # header part
        T_number_of_header = T_number_of_header + 1;
        T_header_part1[T_number_of_header] = part_1;
        T_header_part2[T_number_of_header] = part_2;
        T_header_part3[T_number_of_header] = rest_2;
      } else {
        if (T_number_of_lines == 0) {
          # use it for the first time
          T_number_of_lines = 1;
          T_source_part1[T_number_of_lines] = part_1;
          T_source_part2[T_number_of_lines] = part_2;
          T_source_part3[T_number_of_lines] = rest_2;
        } else {
          if (part_2 > T_number_of_lines) {
            # a new line
            T_number_of_lines = part_2;
            T_source_part1[T_number_of_lines] = part_1;
            T_source_part2[T_number_of_lines] = part_2;
            T_source_part3[T_number_of_lines] = rest_2;
          }
        }
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
    #printf("Line number is %8d\n", part_2);
    #printf("part_1 is >%s<\n", part_1);
    where_ = index(part_1, "-");
    #printf("where_ is   %d\n", where_);
    if (where_ > 0) {
      # don't take care
      #printf("don't take care\n");
    } else {
      where_2 = index(part_1, "#####");
      #printf("where_2 is  %d\n", where_2);
      if (where_2 > 0) {
        # don't take care
        #printf("don't take care\n");
      } else {
        d_part_1 = part_1 + 0;
        #printf("d_part_1 is %d\n", d_part_1);
        # look at T_source_part1[part_2]
        where_3 = index(T_source_part1[part_2], "#####");
        #printf("where_3 is  %d\n", where_3);
        if (where_3 > 0) {
          #printf("copy it ************************************\n");
          #printf("Line number is %8d\n", part_2);
          sum = d_part_1;
          #printf("sum is      %d\n\n", sum);
          # write the sum to T_source_part1
          T_source_part1[part_2] = d_part_1; 
        } else {
          d_T = T_source_part1[part_2] + 0;
          #printf("d_T is      %d\n", d_T);
          sum = d_part_1 + d_T;
          #printf("sum is      %d\n\n", sum);
          # write the sum back to T_source_part1
          T_source_part1[part_2] = sum; 
        }
      }
    }
    #printf("\n");
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
  #printf("SPEICHERN: output_file is %s\n", output_file);
  printf("") > output_file;
  for(i = 1; i <= T_number_of_header; i++) {
    printf("%9s:%5d:%s\n", T_header_part1[i], T_header_part2[i], T_header_part3[i]) >> output_file;
  }
  for (i = 1; i <= T_number_of_lines; i++) {
    printf("%9s:%5d:%s\n", T_source_part1[i], T_source_part2[i], T_source_part3[i]) >> output_file;
  }
  close(output_file);
}
