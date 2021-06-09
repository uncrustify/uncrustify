int   &    aa(int   &   ,int    &   );
// Sp Before Byref Func
//   Sp After Byref Func
//            Sp Before Unnamed Byref
//                    Sp Before Unnamed Byref
int        bb(int    &   x,int    &)
//          Sp Before Byref
//            Sp After Byref
//                    Sp Before Unnamed Byref
{
        return 5;
}
