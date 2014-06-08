//
// Test source for indent_oc_msg_args option.
//
// On wrapped lines, message params should indent depending on value specified
// by indent_oc_msg_args.
//
- (void)someMethod:(NSString *)aString
{
  [self findstart:&startBarcode
    end:&endBarcode
    forLine:greenScalePixels
    derivative:greenDerivative
    centerAt:xAxisCenterPoint
    min:&minValue
    max:&
    maxValue];

  [self findstart:&startBarcode
    end:&endBarcode
    forLine:greenScalePixels
    derivative:greenDerivative
    centerAt:xAxisCenterPoint
    min:&minValue
    max:&maxValue];
}