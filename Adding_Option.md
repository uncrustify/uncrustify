## Add New Config Options
- Branch Creation (Assumed that you have installed git-hub desktop and cloned Unity-Uncrustify repository), make sure that your current branch is master and then create a new branch “Git_hub -> Branch -> New Branch” mention branch- name which is appropriate and publish branch.
- Run all the test cases before starting any development work to ensure that the branch is ERROR free.
- Now you are ready to develop Uncrustify, make sure that your piece of work doesn’t affect the existing functionality which is correct. 

- If you have introduced a new-option then you need to add them in the following  files ```options.cpp``` and ```options.h```, below is the step-by-step procedure
	- Add option in ```options.cpp``` using function ```unc_add_option()``` and add appropriate description for option.
	- Add option in options.h 
		- Build the code to generate uncrustify.exe

		- Copy the EXE from ```\build\debug``` to ```\build\```  folder when you build in debug mode

		- Then run ```\tests\cli\test_cli_options.sh using Git-Bash``` which will generate results folder in ``CLI`` folder ```\tests\cli\``` 
			Note:- You need to add ```CMAKE_BUILD_TYPE:STRING=Release``` in \build\CMakeCache.txt file .

	- Then compare Results ```\tests\cli\results``` and Output folder ```\tests\cli\Output```

	- Merge results to output 

- Check the code and options that you have developed are working without any side effects. 

- Move the Test case which is passed from Staging to Testing

- Run Uncrustify on uncrustify code ```scripts\Run_uncrustify_for_sources.bat``` make sure that the source code is Uncrustified.

- After all the above steps commit the code and raise PR (Pull Request)