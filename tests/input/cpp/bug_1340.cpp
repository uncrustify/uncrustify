double t               = 111; 
double t1                         = 222; 
double t123 = 333;


auto f = [](double x) -> double
         {
             double t                          = 1111;
             double t1              = 1222;
             double t123 = 1333;
         };


std::transform(v1.begin(), v1.end(), v2.begin(),
               [](double x) -> double
               {
                   double t = 2111;
                   double t1                       = 2222;
                   double t123            = 2333;
               }; );
