/**
 * @file d.tokenize.cpp
 * This file gets included into tokenize.cpp.
 * This is specific to the D language.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */


/**
 * Parses all legal D string constants.
 *
 * Quoted strings:
 *   r"Wysiwyg"      # WYSIWYG string
 *   x"hexstring"    # Hexadecimal array
 *   `Wysiwyg`       # WYSIWYG string
 *   'char'          # single character
 *   "reg_string"    # regular string
 *
 * Non-quoted strings:
 * \x12              # 1-byte hex constant
 * \u1234            # 2-byte hex constant
 * \U12345678        # 4-byte hex constant
 * \123              # octal constant
 * \&amp;            # named entity
 * \n                # single character
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a string was parsed
 */
static bool d_parse_string(chunk_t *pc)
{
   if (pc->str[0] == '"')
   {
      return(parse_string(pc, 0, true));
   }
   else if ((pc->str[0] == '\'') ||
            (pc->str[0] == '`'))
   {
      return(parse_string(pc, 0, true));
   }
   else if (pc->str[0] == '\\')
   {
      pc->len = 0;
      while (pc->str[pc->len] == '\\')
      {
         pc->len++;
         /* Check for end of file */
         switch (pc->str[pc->len])
         {
         case 'x':
            /* \x HexDigit HexDigit */
            pc->len += 3;
            break;

         case 'u':
            /* \u HexDigit HexDigit HexDigit HexDigit */
            pc->len += 5;
            break;

         case 'U':
            /* \U HexDigit (x8) */
            pc->len += 9;
            break;

         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
            /* handle up to 3 octal digits */
            pc->len++;
            if ((pc->str[pc->len] >= '0') && (pc->str[pc->len] <= '7'))
            {
               pc->len++;
               if ((pc->str[pc->len] >= '0') && (pc->str[pc->len] <= '7'))
               {
                  pc->len++;
               }
            }
            break;

         case '&':
            /* \& NamedCharacterEntity ; */
            pc->len++;
            while (isalpha(pc->str[pc->len]))
            {
               pc->len++;
            }
            if (pc->str[pc->len] == ';')
            {
               pc->len++;
            }
            break;

         default:
            /* Everything else is a single character */
            pc->len++;
            break;
         }
      }

      if (pc->len > 1)
      {
         pc->type    = CT_STRING;
         cpd.column += pc->len;
         return(true);
      }
   }
   else if (pc->str[1] == '"')
   {
      if ((pc->str[0] == 'r') || (pc->str[0] == 'x'))
      {
         return(parse_string(pc, 1, false));
      }
   }
   return(false);
}
