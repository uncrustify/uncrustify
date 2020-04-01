#################################
Formatting C++ Lambda Expressions
#################################

Uncrustify supports some formatting of c++ lambda expressions, although the support is incomplete.
A complete description of the c++ lambda expressions 
The parts of c++ lambda expressions that is currently recognized by Uncrusitify is:

.. code-block:: c++

  [ captures ] execution_context ( params ) specifiers -> ret { body }
  
Explanations for all the tokens in the above lambda expression (except for execution_context) are found `here <https://en.cppreference.com/w/cpp/language/lambda>`_.
The `execution_context` token is a non-standard addition to allow for specification of the execution space (e.g. host or device in CUDA).
The native specifiers for the `execution_context` for lambda expression in CUDA are ``__device__`` and ``__host__ __device__``.
However it is common for codes to use a preprocessor variable in place of the native specifiers.

The Uncrustify options for formatting of c++ lambda expressions are as follows:

.. code-block:: c++

  # Add or remove space around '=' in C++11 lambda capture specifications.
  #
  # Overrides sp_assign.
  sp_cpp_lambda_assign            = ignore   # ignore/add/remove/force
  
  # Add or remove space after the capture specification of a C++11 lambda when
  # an argument list is present, as in '[] <here> (int x){ ... }'.
  sp_cpp_lambda_square_paren      = ignore   # ignore/add/remove/force
  
  # Add or remove space after the capture specification of a C++11 lambda with
  # no argument list is present, as in '[] <here> { ... }'.
  sp_cpp_lambda_square_brace      = ignore   # ignore/add/remove/force
  
  # Add or remove space after the argument list of a C++11 lambda, as in
  # '[](int x) <here> { ... }'.
  sp_cpp_lambda_paren_brace       = ignore   # ignore/add/remove/force
  
  # Add or remove space between a lambda body and its call operator of an
  # immediately invoked lambda, as in '[]( ... ){ ... } <here> ( ... )'.
  sp_cpp_lambda_fparen            = ignore   # ignore/add/remove/force
  
  # Whether to indent the body of a C++11 lambda.
  indent_cpp_lambda_body          = false    # true/false
  
  # Don't split one-line C++11 lambdas, as in '[]() { return 0; }'.
  nl_cpp_lambda_leave_one_liners  = false    # true/false
  
  # Add or remove newline between C++11 lambda signature and '{'.
  nl_cpp_ldef_brace               = ignore   # ignore/add/remove/force
  
  # The value might be used twice:
  # - at the assignment
  # - at the opening brace
  #
  # To prevent the double use of the indentation value, use this option with the
  # value 'true'.
  #
  # true:  indentation will be used only once
  # false: indentation will be used every time (default)
  indent_cpp_lambda_only_once     = false    # true/false

  
Additionally, a multiple number of  "execution_context" token may be set in the configuration file using the ``EXECUTION_CONTEXT`` token.
Each specification of the ``EXECTION_CONTEXT`` populate a list of allowable keywords to be used at the "execution_context" location.
For example, the following shows setting 4 values (2 default, and 2 custom) for the execution_context.

.. code-block:: c++

  set EXECUTION_CONTEXT __device__
  set EXECUTION_CONTEXT __host__ __device__
  set EXECUTION_CONTEXT DEVICE_LAMBDA_CONTEXT
  set EXECUTION_CONTEXT HOST_DEVICE_LAMBDA_CONTEXT

The effect of these lines in the confiuration file is that any of the strings (__device__, __host__ __device__, DEVICE_LAMBDA_CONTEXT, HOST_DEVICE_LAMBDA_CONTEXT) will be recognized by uncrusitfy and will allow the lambda to be properly identified.

