void MD5::reverse_u32(UINT8 *buf, int n_u32);
MD5::MD5();

class AlignStack
{
public:
	bool m_skip_first;
	AlignStack();
	~AlignStack();
	void End();
};
