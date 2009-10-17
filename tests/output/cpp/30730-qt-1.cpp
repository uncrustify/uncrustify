class Foo : public QObject
{
   Q_OBJECT

 private slots:
   void mySlot();

 public slots:
   void publicSlot();

 signals:
   void somesignal();
};

class foo {
   bool b;
 public:
   int  i;
};
class bar : public
   foo {
   void   *p;
 protected:
   double d;
};

