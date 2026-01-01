// Test case for issue where OC message colon is incorrectly classified
// as CT_COND_COLON when it appears in an OC message call that is
// the outer else branch of a nested ternary expression.
// This causes sp_cond_colon to add a spurious space before the message colon.

// Case 1: Nested ternary with OC message as else branch
static NSString *_GenerateString(BOOL checkA, BOOL checkB, NSInteger componentA, NSInteger componentB) {
	return checkA
    ? checkB
	? [NSString stringWithFormat:@"#%x", (long)componentB]
	: [NSString stringWithFormat:@"%li", (long)componentB]
    : [NSString stringWithFormat:@"#%x", (long)componentA];
}

// Case 2: Ternary in OC message argument affects subsequent OC message colons
@implementation SimpleCellView

- (void)setFileName:(NSString *)fileName
{
	_titleLabel.attributedString =
		[[NSAttributedString alloc]
		 initWithString:fileName
		 attributes:_selected ? _titleAttrsSelected() : _titleAttrsUnselected()];
}

- (void)setSelected:(BOOL)selected
{
	_titleLabel.attributedString =
		[[NSAttributedString alloc]
		 initWithString:_fileName
		 attributes:_selected ? _titleAttrsSelected() : _titleAttrsUnselected()];
}

- (void)loadView
{
	self.view = [[SimpleCellView alloc] initWithFrame:NSZeroRect];
}

@end
