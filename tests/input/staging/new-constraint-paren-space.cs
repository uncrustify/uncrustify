// same as 10130-sp_between_new_paren.cs
T F<T>() where T : new()
{
	return new         T();
}
