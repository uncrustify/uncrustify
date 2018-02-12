int ff()
{
	// not a lambda fcn so don't surround "->" by spaces
	f()[0]->size();
	if(true) {
		return 1;
	}
}
