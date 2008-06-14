struct X {
void operator-(int);
void operator+(int);
void operator()();
};
/* *INDENT-OFF* */
  struct Y {
    void operator-(int){}


   void operator+(int){}  \
    void operator()(){}

  };
/* *INDENT-ON* */
struct Y {
void operator-(int){}
void operator+(int){}
void operator()(){}
};

