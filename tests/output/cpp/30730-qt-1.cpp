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
