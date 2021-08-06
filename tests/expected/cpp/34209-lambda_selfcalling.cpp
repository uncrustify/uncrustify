void f(){
	int i = 0;
	const auto j = [](int k){
			       return k+2;
		       }       (i);

	const auto l = ([](int k){
		return k+2;
	})                     (i);
}