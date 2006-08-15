/**
 * @file d.tokenize.cpp
 * This file gets included into tokenize.cpp.
 * This is specific to the D language.
 *
 * $Id$
 */


/**
 * Count the number of characters in the number.
 * The next bit of text starts with a number (0-9 or '.'), so it is a number.
 * Count the number of characters in the number.
 *
 * This should cover all D number formats.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a number was parsed
 */
static bool d_parse_number(chunk_t *pc)
{
   int  len;
   int  tmp;
   bool is_float;
   bool did_hex = false;


   if (!isdigit(pc->str[0]) &&
       ((pc->str[0] != '.') || !isdigit(pc->str[1])))
   {
      return(false);
   }
   len = 1;

   is_float = (pc->str[0] == '.');

   /* Check for Hex, Octal, or Binary */
   if (pc->str[0] == '0')
   {
      switch (toupper(pc->str[1]))
      {
      case 'X':               /* hex */
         did_hex = true;
         do
         {
            len++;
         } while (isxdigit(pc->str[len]) || (pc->str[len] == '_'));
         break;

      case 'B':               /* binary */
         do
         {
            len++;
         } while ((pc->str[len] == '0') || (pc->str[len] == '1') ||
                  (pc->str[len] == '_'));
         break;

      case '0':                /* octal */
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
         do
         {
            len++;
         } while (((pc->str[len] >= '0') && (pc->str[len] <= '7')) ||
                  (pc->str[len] == '_'));
         break;

      default:
         /* either just 0 or 0.1 or 0UL, etc */
         break;
      }
   }
   else
   {
      /* Regular int or something */
      while (isdigit(pc->str[len]) || (pc->str[len] == '_'))
      {
         len++;
      }
   }

   /* Check is we stopped on a decimal point */
   if (pc->str[len] == '.')
   {
      len++;
      is_float = true;
      if (did_hex)
      {
         while (isxdigit(pc->str[len]) || (pc->str[len] == '_'))
         {
            len++;
         }
      }
      else
      {
         while (isdigit(pc->str[len]) || (pc->str[len] == '_'))
         {
            len++;
         }
      }
   }

   /* Check exponent */
   tmp = toupper(pc->str[len]);
   LOG_FMT(LSYS, "Exp: %c\n", pc->str[len]);
   if ((tmp == 'E') || (tmp == 'P'))
   {
      is_float = true;
      len++;
      if ((pc->str[len] == '+') || (pc->str[len] == '-'))
      {
         len++;
      }
      while (isdigit(pc->str[len]) || (pc->str[len] == '_'))
      {
         len++;
      }
   }

   if ((pc->str[len] == 'i') || (toupper(pc->str[len]) == 'F'))
   {
      is_float = true;
   }

   if (is_float)
   {
      /* Check float suffixes */
      if ((pc->str[len] == 'L') || (toupper(pc->str[len]) == 'F'))
      {
         len++;
      }
      if (pc->str[len] == 'i')
      {
         len++;
      }
   }
   else
   {
      /* Check integer suffixes (do twice) */
      if ((pc->str[len] == 'L') || (toupper(pc->str[len]) == 'U'))
      {
         len++;
      }
      if ((pc->str[len] == 'L') || (toupper(pc->str[len]) == 'U'))
      {
         len++;
      }
   }

   pc->len     = len;
   pc->type    = is_float ? CT_NUMBER_FP : CT_NUMBER;
   cpd.column += len;
   return(true);
}


/**
 * Strings in D can start with:
 * r"Wysiwyg"
 * x"hexstring"
 * `Wysiwyg`
 * 'char'
 * "reg_string"
 * \'
 * The next bit of text starts with a quote char " or ' or <.
 * Count the number of characters until the matching character.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a string was parsed
 */
static bool d_parse_string(chunk_t *pc)
{
   if (pc->str[0] == '"')
   {
      return parse_string(pc, 0, true);
   }
   else if ((pc->str[0] == '\'') ||
            (pc->str[0] == '`'))
   {
      return parse_string(pc, 0, false);
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
            /* \x HexDigit HexDigit HexDigit HexDigit */
            pc->len += 5;
            break;

         case 'U':
            /* \x HexDigit (x8) */
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
         return true;
      }
   }
   else if (pc->str[1] == '"')
   {
      if ((pc->str[0] == 'r') || (pc->str[0] == 'x'))
      {
         return parse_string(pc, 1, false);
      }
   }
   return(false);
}
