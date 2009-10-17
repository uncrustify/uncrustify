
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

    return 0;
}
