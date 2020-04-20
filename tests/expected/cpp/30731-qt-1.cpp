class Foo : public QObject
{
	Q_OBJECT


 private slots:

	void mySlot() {
	}


 public slots:

	void publicSlot();


 signals:

	void somesignal();

};

class foo
{
	bool b;


 public:

	int i;
};
class bar : public
	foo
{
	void*p;


 protected:

	double d;
	enum e {A,B};


 private:
};

class Foo1 : public QObject
{
	Q_OBJECT


 private Q_SLOTS:

	void mySlot();


 public Q_SLOTS:

	void publicSlot();


 Q_SIGNALS:

	void somesignal();
};

class foo1
{
	bool b;


 public:

	int i;
};
class bar : public
	foo1
{
	void*p;


 protected:

	double d;
};

