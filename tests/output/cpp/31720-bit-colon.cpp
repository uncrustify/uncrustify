class C
{
 public:
   size_t f1 : 1;
   size_t f1 : 1;
   size_t f2 : sizeof(size_t) - 1;

 Q_SIGNALS:
   void somesignal();
};

struct S
{
 private:
   size_t f1 : 1;
   size_t f1 : 1;
   size_t f2 : sizeof(size_t) - 1;

 Q_SIGNALS:
   void somesignal();
};
