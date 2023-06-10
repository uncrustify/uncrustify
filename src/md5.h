/**
 * @file md5.h
 * A simple class for MD5 calculation
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef MD5_H_INCLUDED
#define MD5_H_INCLUDED

#include "base_types.h"

class MD5
{
public:
   MD5();


   ~MD5() = default;


   /**
    * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
    * initialization constants.
    */
   void Init();


   /**
    * Update context to reflect the concatenation of another buffer full
    * of bytes.
    */
   void Update(const void *data, UINT32 len);


   /**
    * Final wrapup - pad to 64-byte boundary with the bit pattern
    * 1 0* (64-bit count of bits processed, MSB-first)
    *
    * @param[out] digest  calculated MD5 checksum
    */
   void Final(UINT8 digest[16]);


   /**
    * The core of the MD5 algorithm, this alters an existing MD5 hash to
    * reflect the addition of 16 longwords of new data.  MD5::Update blocks
    * the data and converts bytes into longwords for this routine.
    */
   static void Transform(UINT32 buf[4], UINT32 in_data[16]);


   /**
    * Calculates MD5 for a block of data
    *
    * @param      data    data to calculate MD5 for
    * @param      length  number of bytes in data
    * @param[out] digest  calculated MD5 checksum
    */
   static void Calc(const void *data, UINT32 length, UINT8 digest[16]);


private:
   UINT32 m_buf[4];
   UINT32 m_bits[2];
   UINT32 m_in32[16];
   // Alternate view of m_in32
   UINT8  *m_in8;
   bool   m_need_byteswap;
   bool   m_big_endian;


   /**
    * Reverse the bytes in 32-bit chunks.
    * 'buf' might not be word-aligned.
    *
    * @param buf    The byte array to reverse
    * @param n_u32  The number of UINT32's in the data
    */
   void reverse_u32(UINT8 *buf, int n_u32);
};

#endif /* MD5_H_INCLUDED */
