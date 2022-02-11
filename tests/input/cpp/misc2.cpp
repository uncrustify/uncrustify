/*
I tried to modify the spaces when using casts like static_cast etc. by
using sp_before_angle, sp_after_angle and sp_inside_angle. Even setting
all of those options to remove results in the following:
*/

myvar = dynamic_cast < MyClass<T>* > (other);
// expected:
//myvar = dynamic_cast<MyClass<T>*>(other);

/*
Sometime pointers and references are still not detected correctly in
special cases - i guess.
*/
//When using "sp_before_ptr_star = remove" the result is:
typedef std::list<StreamedData *>::iterator iterator;
//typedef std::list<StreamedData *>::iterator iterator;
//------------------------------^ This space show not be there

typedef void (T::*Routine)(void);

//Similar with "sp_before_byref = remove":
unsigned long allocate(unsigned long size, void* & p);
//unsigned long allocate(unsigned long size, void* & p);
//------------------------------------------------^ The same here

void foo(void)
{
   List<byte>bob = new List<byte> ();

   /* Align assignments */
   align_assign(Chunk::GetHead(),
                cpd.settings[UO_align_assign_span].n,
                cpd.settings[UO_align_assign_thresh].n);
}

Args::Args(int argc, char **argv)
{
   m_count = argc;
   m_values = argv;
   int len = (argc >> 3) + 1;
   m_used = new UINT8[len];
   if (m_used != NULL)
   {
      memset(m_used, 0, len);
   }
}

void Args(int argc, char **argv)
{
   m_count = argc;
   m_values = argv;
   int len = (argc >> 3) + 1;
   m_used = new UINT8[len];
   if (m_used != NULL)
   {
      memset(m_used, 0, len);
   }
}
