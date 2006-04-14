/**
 * @file args.d
 * A Class to parse command-line arguments
 *
 * $Id$
 */
module uncrustify.args;

class Args
{
   char [][]   m_args;
   bool []     m_used;

   this(char [][] args)
   {
      Set(args);
   }

   void Set(char [][] args)
   {
      m_args = args;
      m_used.length = args.length;
   }

   void Clear()
   {
      m_args.length = 0;
      m_used.length = 0;
   }

   void Append(char [] arg)
   {
      m_args.length = m_args.length + 1;
      m_args[m_args.length - 1] = arg;
      m_used.length = m_args.length;
   }

   void SetUsed(int idx)
   {
      if ((idx >= 0) && (idx < m_used.length))
      {
         m_used[idx] = true;
      }
   }

   bool GetUsed(int idx)
   {
      if ((idx >= 0) && (idx < m_used.length))
      {
         return m_used[idx];
      }
      return false;
   }

   char [] GetUnused(inout int index)
   {
      int idx = index;
      if (idx < 0)
      {
         idx = 0;
      }
      while (idx < m_used.length)
      {
         if (!m_used[idx])
         {
            index = idx + 1;
            return m_args[idx];
         }
         idx++;
      }
      index = idx + 1;
      return null;
   }

   char [] GetArg(int idx)
   {
      if ((idx >= 0) && (idx < m_args.length))
      {
         return m_args[idx];
      }
      return null;
   }

   void SetArg(int idx, char [] arg)
   {
      if ((idx >= 0) && (idx < m_args.length))
      {
         m_args[idx] = arg;
      }
   }

   bool Present(char [] token, bool exact = true)
   {
      char [] arg;
      for (int idx = 0; idx < m_args.length; idx++)
      {
         arg = m_args[idx];

         if (exact)
         {
            if ((arg.length == token.length) && (arg == token))
            {
               SetUsed(idx);
               return(true);
            }
         }
         else
         {
            if ((arg.length >= token.length) &&
                (arg[0..token.length] == token))
            {
               SetUsed(idx);
               return(true);
            }
         }
      }
      return(false);
   }

   char [] Param(char [] token)
   {
      int index = 0;
      return Params(token, index);
   }

   char [] Params(char [] token, inout int index)
   {
      char [] arg;
      for (int idx = index; idx < m_args.length; idx++)
      {
         arg = m_args[idx];

         if ((arg.length >= token.length) &&
             (arg[0..token.length] == token))
         {
            if (arg.length > token.length)
            {
               SetUsed(idx);
               index = idx + 1;
               return (arg[token.length .. arg.length]);
            }
            if ((idx + 1) < m_args.length)
            {
               SetUsed(idx);
               SetUsed(idx + 1);
               index = idx + 2;
               return (m_args[idx + 1]);
            }
            return ("");
         }
      }
      index =  m_args.length;
      return(null);
   }
}

