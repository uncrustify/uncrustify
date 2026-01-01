// Test case for regression where consecutive ternary expressions in OC message
// arguments cause the second ternary to break incorrectly with ? on its own line.
// This is triggered when:
// 1. Two consecutive ternary expressions in OC message parameters
// 2. Long function call as the ternary condition
// 3. Longer parameter name for the first ternary

static ThumbnailMetadata *_thumbnailMetadataForMetadataType(ThumbnailMetadataType metadataType,
                                                            id<Provider> provider,
                                                            id<LuggageCheck> luggageCheck) {
	switch (metadataType) {
	case ThumbnailMetadataTypeFirst:
		return [ThumbnailMetadata metadataWithReallyLongMethodNameAndArg1:provider.model.arg1
		        arg2:provider.arg2
		        arg3:provider.arg3.integerValue
		        lastArg:this_is_a_really_really_really_really_long_function_to_do_a_boolean_check_with_a_provider(luggageCheck) ? nil : provider.lastArg];
	case ThumbnailMetadataSecond:
		return [ThumbnailMetadata metadataWithReallyLongMethodNameAndArg1:provider.model.arg1
		        arg2:provider.arg2
		        arg3:provider.arg3.integerValue
		        arg4WithLongerName:this_is_a_really_really_really_really_long_function_to_do_a_boolean_check_with_a_provider(luggageCheck) ? provider.arg4.stringValue : nil
		        lastArg:this_is_a_really_really_really_really_long_function_to_do_a_boolean_check_with_a_provider(luggageCheck) ? nil : provider.lastArg];
	case ThumbnailMetadataThird:
		return [ThumbnailMetadata metadataWithReallyLongMethodNameAndArg1:provider.model.arg1
		        arg2:provider.arg2
		        arg3:provider.alternativeArg3.integerValue
		        lastArg:this_is_a_really_really_really_really_long_function_to_do_a_boolean_check_with_a_provider(luggageCheck) ? nil : provider.lastArg];
	case ThumbnailMetadataFourth:
		return [ThumbnailMetadata metadataWithReallyLongMethodNameAndArg1:provider.model.arg1
		        arg2:provider.arg2
		        arg3:provider.alternativeArg3.integerValue
		        arg4WithLongerName:this_is_a_really_really_really_really_long_function_to_do_a_boolean_check_with_a_provider(luggageCheck) ? provider.arg4.stringValue : nil
		        lastArg:this_is_a_really_really_really_really_long_function_to_do_a_boolean_check_with_a_provider(luggageCheck) ? nil : provider.lastArg];
	default:
		abort();
	}
}
