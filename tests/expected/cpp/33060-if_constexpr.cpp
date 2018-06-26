static constexpr int test{
	if constexpr (condition_1)
		return 1;
	else if constexpr (condition_2)
		return 2;
	else
		return 3;
};
