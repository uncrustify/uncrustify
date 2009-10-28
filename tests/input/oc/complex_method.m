// Turn the document contents into a single savable lump of data
- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError {
#pragma unused(typeName)
    
    // Produce the data lump:
    NSData * retval = [NSKeyedArchiver archivedDataWithRootObject:model];
    
    // If the lump is nil something went wrong
    // fill out the error object to explain what wrent wrong
    if ( outError != NULL ) {
        // The sender wanted an error reported. If there
        // was a problem, fill in an NSError object
        if (retval == nil) {
            // The error object should include an (unhelpful) 
            // explanation of what happened
            NSDictionary * userInfoDict = [NSDictionary dictionaryWithObjectsAndKeys:
                                               @"Internal error formatting data", NSLocalizedDescriptionKey, 
                                               @"Archiving of data failed. Probably a bug.", NSLocalizedFailureReasonErrorKey, 
                                               @"There's nothing you can do.", NSLocalizedRecoverySuggestionErrorKey, nil];
            
            *outError = [NSError errorWithDomain:LinearInternalErrorDomain
                                                code:linErrCantFormatDocumentData 
                                                userInfo:userInfoDict];
        } else {
            // No problem. Don't supply an error object.
            *outError = nil;
        }
	}
	return retval;
}
