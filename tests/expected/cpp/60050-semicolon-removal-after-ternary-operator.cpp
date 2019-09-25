std::string StrGet()
{
	return IsConnected() ? "Connected" : {};
}

std::string StrGet2()
{
	return !IsConnected() ? {} : "Connected";
}
