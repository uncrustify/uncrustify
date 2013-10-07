
NSDictionary *dictionary = @{
   @0: @"red", @1: @"green", @2: @"blue"
};

NSArray *array = @[@0, @1, @2, @YES, @'Z', @42U];

NSArray *multilineArray = @[
   @0, @1, @2, @YES,
   @'Z', @42U
];

void main(int argc, const char *argv[])
{
   // character literals.
   NSNumber *theLetterZ = @'Z';         // equivalent to [NSNumber numberWithChar:'Z']

   // integral literals.
   NSNumber *fortyTwo         = @42;    // equivalent to [NSNumber numberWithInt:42]
   NSNumber *fortyTwoUnsigned = @42U;   // equivalent to [NSNumber numberWithUnsignedInt:42U]
   NSNumber *fortyTwoLong     = @42L;   // equivalent to [NSNumber numberWithLong:42L]
   NSNumber *fortyTwoLongLong = @42LL;  // equivalent to [NSNumber numberWithLongLong:42LL]

   // floating point literals.
   NSNumber *piFloat  = @3.141592654F;  // equivalent to [NSNumber numberWithFloat:3.141592654F]
   NSNumber *piDouble = @3.1415926535;  // equivalent to [NSNumber numberWithDouble:3.1415926535]

   // BOOL literals.
   NSNumber *yesNumber = @YES;          // equivalent to [NSNumber numberWithBool:YES]
   NSNumber *noNumber  = @NO;           // equivalent to [NSNumber numberWithBool:NO]

#ifdef __cplusplus
   NSNumber *trueNumber  = @true;       // equivalent to [NSNumber numberWithBool:(BOOL)true]
   NSNumber *falseNumber = @false;      // equivalent to [NSNumber numberWithBool:(BOOL)false]
#endif
}
