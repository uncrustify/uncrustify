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
   ~MD5() { }

   void Init();
   void Update(const void *data, UINT32 len);

   void Final(UINT8 digest[16]);

   /* internal function */
   static void Transform(UINT32 buf[4], UINT32 in_data[16]);

   static void Calc(const void *data, UINT32 length, UINT8 digest[16]);

private:
   UINT32 m_buf[4];
   UINT32 m_bits[2];
   UINT8  m_in[64];
   bool   m_need_byteswap;
   bool   m_big_endian;

   void reverse_u32(UINT8 *buf, int n_u32);
};

#endif /* MD5_H_INCLUDED */
