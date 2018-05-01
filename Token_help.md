## Token Generation
- In cmake test suit we have integrated a new feature to generate token and parent token and the compare the output and result file. If both the files are same then test case will pass otherwise testcase will be failed.
- Generating Token files 
- we can generate Token files by using the  command line option ```--token``` followed by token file name
	- Eg: ``` uncrustify.exe -c Uncrustify.Common-CStyle.cfg -p debug.txt -f input.cpp -o output.cpp --token Tokens.txt -La  > a.log 2>&1```
	
	 ```
		 Line [Token]             Text
		 1    [PREPROC]           #
		 1    [PP_INCLUDE]        include
		 1    [PREPROC_BODY]      "macros.h"
		 1    [NEWLINE]           
		 3    [FUNC_CALL]         MACRO_DEFINED_ELSEWHERE
		 3    [FPAREN_OPEN]       (
		 3    [WORD]              Argument
		 3    [FPAREN_CLOSE]      )
		 3    [BRACE_OPEN]        {
		 3    [NEWLINE]           
		 4    [BRACE_CLOSE]       }```
		 

- we can generate Parent_Token files by using the  command line option ```--parent``` followed by token file name
	- Eg: ``` uncrustify.exe -c Uncrustify.Common-CStyle.cfg -p debug.txt -f input.cpp -o output.txt --parent parent_token.txt -La  > a.log 2>&1```
		
		```
			Line [Parent]            Text
			1    [PP_INCLUDE]        #
			1    [NONE]              include
			1    [NONE]              "macros.h"
			1    [NONE]              
			3    [NONE]              MACRO_DEFINED_ELSEWHERE
			3    [FUNC_CALL]         (
			3    [NONE]              Argument
			3    [FUNC_CALL]         )
			3    [FUNC_CALL]         {
			3    [NONE]              
			4    [FUNC_CALL]         }
		```

- we can generate generate Tokens & Parent Tokens on one go by enabling both --token,--parent
	- Eg: ``` uncrustify.exe -c Uncrustify.Common-CStyle.cfg -p debug.txt -f ip.cpp -o op.txt --token Tokens.txt --parent parent_token.txt -La  > a.log 2>&1```

### Cmake Changes for RUN_TESTS
- If we want to generate tokens,parent tokens for the test cases then we need to change in 
	- ```\tests\CMakeLists.txt```

- Add languages to output_test_lang to do output comparison on those languages
	- ```set(output_test_lang c-sharp c cpp d java pawn objective-c vala ecma imported regression)```

- Add languages to token_test_lang to do token comparison on those languages
	- ```set(token_test_lang c-sharp c cpp d java pawn objective-c vala ecma imported regression)```

- Add languages to ptoken_test_lang to do parent token comparison on those languages
	- ```set(ptoken_test_lang c-sharp c cpp d java pawn objective-c vala ecma imported regression)```
- if we want to generate Tokens (or) Parent_Tokens for specific languages then we need to pass only those languages
	- ```set(ptoken_test_lang vala)```
	- The above piece of code will generate Parent Tokens for only the cases that are present in Vala, we have to follow the same to generate Tokens as well.