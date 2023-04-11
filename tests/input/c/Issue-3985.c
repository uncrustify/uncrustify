int keep_symbol()
{
			if(
				#ifdef HAVE_BFD_2_34
				bfd_section_size() == 0
				#else
				bfd_get_section_size() == 0
				#endif
			)
			{
				return false;
			}
	return true;
}
