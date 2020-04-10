#################################
Formatting C++ Lambda Expressions
#################################

Uncrustify supports some formatting of c++ lambda expressions, although 
the support is incomplete.
The parts of c++ lambda expressions that are currently recognized by Uncrusitify
are:

.. code-block:: c++

  [ captures ] execution_context ( params ) specifiers -> ret { body }
  
Explanations for all the tokens in the above lambda expression
(except for ``execution_context``),
as well as a complete description of c++ lambda expressions,
are found `here <https://en.cppreference.com/w/cpp/language/lambda>`_.
The ``execution_context`` token is a non-standard addition to allow for 
specification of the execution space (e.g. host or device in CUDA).
The native specifiers for the `execution_context` for lambda expression in CUDA 
are ``__device__`` and ``__host__ __device__``.
However, it is common for code to use a preprocessor variable in place of the 
native specifiers.

The Uncrustify options for formatting of c++ lambda expressions are:

.. code-block::

  sp_cpp_lambda_assign
  sp_cpp_lambda_square_paren
  sp_cpp_lambda_square_brace
  sp_cpp_lambda_paren_brace
  sp_cpp_lambda_fparen
  nl_cpp_lambda_leave_one_liners
  nl_cpp_ldef_brace
  indent_cpp_lambda_body
  indent_cpp_lambda_only_once

Please refer to the example configuration file at 
`uncrustify/documentation/htdocs/default.cfg <https://github.com/uncrustify/uncrustify/blob/master/documentation/htdocs/default.cfg>`_ 
for an explanation of the options.
Additionally, a multiple number of ``execution_context`` tokens may be set in 
the configuration file:

.. code-block::

  set EXECUTION_CONTEXT __host__ __device__
  set EXECUTION_CONTEXT DEVICE_LAMBDA_CONTEXT HOST_DEVICE_LAMBDA_CONTEXT

The effect of these lines in the configuration file is that any of the strings
(``__host__``, ``__device__``,
``DEVICE_LAMBDA_CONTEXT``, ``HOST_DEVICE_LAMBDA_CONTEXT``)
will be recognized by uncrusitfy
and will allow the lambda to be properly identified.
Note that each word after the token name
(``EXECUTION_CONTEXT`` in this instance) is a separate token.
This means that uncrustify will parse ``__host__`` and ``__device__``
as separate tokens, and there is no need to specify ``__device__`` twice.
