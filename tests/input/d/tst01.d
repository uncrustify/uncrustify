package void writeRegister(int aRegisterOffset, ushort aValue)
in {
    assert(aRegisterOffset >= 0);
    assert(aRegisterOffset < IMAGE_SIZE);
} body {
int  idx = aRegisterOffset / 2;
         mMemCache[idx] = aValue;
         uint readback;
         uint st;
         uint st2;
         volatile {
             mMemImage[idx] = aValue;
             //readback = (cast(uint*)mMemImage.ptr)[ idx/2 ];
             //st = mMemImage[ 0x28/2 ];
             //st2 = mMemImage[ 0x2A/2 ];
         }
         //if( aValue != readback )
         {
             //debug(IRQ) writefln( "writeRegister %04x, %04x", aRegisterOffset, aValue);
         }
         // comment
}
//
