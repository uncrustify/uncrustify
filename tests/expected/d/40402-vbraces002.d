int super_fun(bool a, bool b, bool c, bool d)
{
	int i = 6;
	static if (true)
		while (true)
			if(b) {
				return 1;
			}
			else if (c) {
				while (true)
					if(d) {
						return 2;
					}
					else{
						while (true)
							if(a)
								return 3;
					}
			}
	while (d)
		return 4;
	return 1;
}