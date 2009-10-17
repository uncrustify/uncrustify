        package static ushort calcHeaderCrc(bool aStartupFrame, bool aSyncFrame, ushort aFrameId, ushort aPayloadWords) {
            uint crcData = aPayloadWords;

            crcData |= (aFrameId << 7);

            if (aSyncFrame) {
                crcData |= BIT_19;
            }

            if (aStartupFrame) {
                crcData |= BIT_18;
            }

            ushort       crc       = 0x1a;
            const ushort table[16] = [
                                         0x0000, 0x0385, 0x070A, 0x048F,
                                         0x0591, 0x0614, 0x029B, 0x011E,
                                         0x00A7, 0x0322, 0x07AD, 0x0428,
                                         0x0536, 0x06B3, 0x023C, 0x01B9];

            for (int i = 0; i < 5; ++i) {
                if (i != 0) {
                    crcData <<= 4;
                }
                crc = ((crc << 4) & 0x7FF) ^ table[((crc >> 7) ^ (crcData >> 16)) & 0x0F];
            }

            return(crc);
        }
 
