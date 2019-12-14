class MyClass {
	void foo(List<Integer> arr) {
		arr.forEach(n -> {
			// Okay: This line will be indented with only tabs.
			if (cond1) { // Okay
				// BAD1: This line will be indented with tabs up to lambda brace level, then spaces for the rest.
				if (cond2) // BAD2
					// Okay
					bar(); // Okay
				if (cond3) // BAD3
				{ // BAD4
					// BAD5
					bar(); // BAD6
				} // Okay
			} // Okay
			if (cond4) { // Okay
				/*
				BAD7: C-style comments will also be affected on all lines.
				*/
			} // Okay
			if (cond5) // Okay
			{ // Okay
				bar(); // BAD8
			} // Okay
			if (cond6) // Okay
				bar; // Okay
		});
	}
}
