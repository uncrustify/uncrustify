/**
 * @file char_table.h
 * A simple table to help tokenize stuff.
 * Used to parse strings (paired char) and words.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef CHAR_TABLE_H_INCLUDED
#define CHAR_TABLE_H_INCLUDED


/**
 * bit0-7 = paired char
 * bit8 = OK for keyword 1st char
 * bit9 = OK for keyword 2+ char
 */
struct CharTable
{
   static int chars[128];

   enum
   {
      KW1 = 0x0100,
      KW2 = 0x0200,
   };

<<<<<<< HEAD
   static inline int Get(int ch)
   {
      if (ch < 0)
      {
         return 0;
      }
      if (ch < (int)ARRAY_SIZE(chars))
      {
         return chars[ch];
=======

   static inline int Get(int ch)
   {
      if (ch < 0)
      {
         return(0);
      }
      if (ch < (int)ARRAY_SIZE(chars))
      {
         return(chars[ch]);
>>>>>>> uncrustify/master
      }

      /* HACK: If the top bit is set, then we are likely dealing with UTF-8,
       * and since that is only allowed in identifiers, then assume that is
       * what this is. This only prevents corruption, it does not properly
       * handle UTF-8 because the byte length and screen size are assumed to be
       * the same.
       */
      return(KW1 | KW2);
   }


   static inline bool IsKw1(char ch)
   {
      return((Get(ch) & KW1) != 0);
   }


   static inline bool IsKw2(char ch)
   {
      return((Get(ch) & KW2) != 0);
   }
};

#ifdef DEFINE_CHAR_TABLE
int CharTable::chars[128] =
{
   0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,   /* [........] */
   0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,   /* [........] */
   0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,   /* [........] */
   0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,   /* [........] */
   0x000, 0x000, 0x022, 0x000, 0x300, 0x000, 0x000, 0x027,   /* [ !"#$%&'] */
   0x029, 0x028, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,   /* [()*+,-./] */
   0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200,   /* [01234567] */
   0x200, 0x200, 0x000, 0x000, 0x03e, 0x000, 0x03c, 0x000,   /* [89:;<=>?] */
   0x200, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,   /* [@ABCDEFG] */
   0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,   /* [HIJKLMNO] */
   0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,   /* [PQRSTUVW] */
   0x300, 0x300, 0x300, 0x05d, 0x000, 0x05b, 0x000, 0x300,   /* [XYZ[\]^_] */
   0x060, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,   /* [`abcdefg] */
   0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,   /* [hijklmno] */
   0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,   /* [pqrstuvw] */
   0x300, 0x300, 0x300, 0x07d, 0x000, 0x07b, 0x000, 0x000,   /* [xyz{|}~.] */
};
#endif /* DEFINE_CHAR_TABLE */

#endif /* CHAR_TABLE_H_INCLUDED */
