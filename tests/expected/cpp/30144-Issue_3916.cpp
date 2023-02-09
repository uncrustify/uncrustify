#include <string>

using TEnglishString = std::string;

class CComCommandInfo
{
public:
CComCommandInfo( TEnglishString, TEnglishString );
};

template< typename T >
class CGenericCommandEx
{
public:
CGenericCommandEx( T );
};


template< typename T >
void Test( TEnglishString commandName_, TEnglishString commandDescription_, T functor_ )
{
        CComCommandInfo cmdInfo( std::forward< TEnglishString >( commandName_ ),
                                 std::forward< TEnglishString >( commandDescription_ ) );

        auto* pCommand =
                new CGenericCommandEx(
                        std::forward< decltype( functor_ ) >( functor_ ) ); // <--- Note the extra spaces added here
}

