
int main( int argc, const char *argv[] ) {
	@try {
		[NSException raise:NSInternalInconsistency
					format:@"An internal inconsistency was raised"];
	}
	@catch (NSException *e) {
		NSLog(@"Catch");
	}
	@finally {
		NSLog(@"Finally");
	}
    
 @throw [NSException exceptionWithName:@"foo" reason:@"bar" userInfo:nil];

NSException *exception = [NSException exceptionWithName: @"HotTeaException"
                          reason: @"The tea is too hot"
                          userInfo: nil];

@throw exception;

    return 0;
}
