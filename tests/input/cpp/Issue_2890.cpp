#include <iostream>
#include <sstream>
#include <string>

struct StringBuilder
{
        template <typename T>
        StringBuilder& append(const T& thing)
        {
                ss << thing;
                return *this;
        }
        std::string build()
        {
                return ss.str();
        }
        std::stringstream ss;
};

int main()
{
        std::string my_____String = StringBuilder().append(7).append(" + ").append(21).append(" = ").append(7 + 21).build();
        std::string my_____String = StringBuilder()
                .append(7)
                .append(" + ")
                .append(21)
                .append(" = ")
                .append(7 + 21)
                .build();

      std::cout << my___String << std::endl;
}

void function()
{
      auto response = ResponseBuilder_1(1)
         .setStatus_1(status)
         .finish_1();

    ResponseBuilder_2(request)
        .setStatus_2(status)
        .finish_2();

          return ResponseBuilder_3(request).setStatus_3(status).finish_3();
}
