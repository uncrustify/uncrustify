static int ConvertEndian(void *ptr, int bytes)
{
	switch(bytes)
	{
		case 2:
		{
			uint16_t *value = (uint16_t *) ptr;
			
			*value = bswap_16(*value);
		}
		return 1;
		case 4:
		{
			uint32_t *value = (uint32_t *) ptr;
			
			*value = bswap_32(*value);
		}
		return 1+
			2+3;

		case 8:
		{
			uint64_t *value = (uint64_t *) ptr;
			
			*value = bswap_64(*value);
		}

		return 1+

			2

			+fn(

				x
			);
		// comment
		default:
			break;
	}
	return 0;
}
