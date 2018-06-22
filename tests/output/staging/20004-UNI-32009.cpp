//TestCase-001
void GetCharacterRenderInfo(unsigned int charCode, int size, unsigned int style, float pixelsPerPoint, Rectf& verts, Rectf& uvs, bool &flipped, unsigned int* error = NULL) const;

//TestCase-002
void AddCharacterInfoEntry(const Rectf& uv, const Rectf& vert, float advance, int character, bool flipped, int size, unsigned int style, float pixelsPerPoint);

//TestCase-003
bool CacheFontForText(UInt16 *chars, int length, int size = 0, float pixelsPerPoint = 1.0f, unsigned int style = kStyleDefault, const dynamic_array<TextFormatChange> *formats = NULL);

//TestCasse-004
FontImpl(MemLabelId label, TextRendering::Font* owningFont);
