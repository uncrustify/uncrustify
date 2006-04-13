bool foo(int & idx)
{
   if (idx < m_count)
   {
      idx++;
      return m_bool[idx-1];
   }
   return false;
}

