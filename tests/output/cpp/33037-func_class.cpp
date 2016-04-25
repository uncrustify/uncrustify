/**
 * Reverse the bytes in 32-bit chunks.
 */
void MD5::reverse_u32(UINT8 *buf, int n_u32)
{
	UINT8 tmp;
}


MD5::MD5()
{
	m_buf[0] = 0x01020304;
}

class AlignStack
{
public:
	bool m_skip_first;


	AlignStack()
	{
	}


	~AlignStack()
	{
	}


	void End()
	{
	}
};
