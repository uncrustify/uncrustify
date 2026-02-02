// Test case for issue where case label colons were incorrectly classified
// as CT_COND_COLON when the file contains multiple ternary operators.
// This caused the colon to be moved to a new line with pos_conditional = lead.
// Both ternary functions below are required to trigger the bug.

static ButtonProps _InviteButtonComponent(void)
{
	return ButtonProps {
		       .action = condition
		                 ? sAction1
		                 : sAction2
	};
}

static NSString *_ShowErrorAlert(NSString *message)
{
	return message
	       ?: FBT("Sorry, something went wrong. Please try again.", "Dialog text showing user error that we couldn't send an invite for whatever reason.");
}

static NSString *_ShowErrorAlertWithComment(NSString *message)
{
	return message
	       ? /* comment */ : @"default";
}

static BOOL _IsEmailInvite(CellType inviteCellType)
{
	switch (inviteCellType) {
	case CellTypeEmailSuggestionContact:
	case CellTypeEmailAddressBookContact:
	case CellTypeSuggestedContact:
		return YES;
	case CellTypePhoneSuggestionContact:
	case CellTypePhoneAddressBookContact:
		return NO;
	default:
		abort();
	}
}
