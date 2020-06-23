namespace ComponentSpec {
void build(Context c)
{
	if (index == NSNotFound) {
		return;
	}

	invokeUpdateInvitees(c, invitees);
}
}
