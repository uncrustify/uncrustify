// Use case from issue #1161
class test
{
   // comment 1 (gets methods)
   public:
   // get 1
   int get1();
   // get 2
   int get2();




                          // comment 2 (sets methods)
                          public:
                          // set 1
                          int set1();
                          // set2
                          int set2();

};

// Use cases from issue #2704
class Foo
{
public:
    /// @name Constructors
    /// @{

        Foo(int value) : value_(value)
        {}

            /// @}

private:
    int value_;
};

class Bar
{
public:
            /*!
             * @name Constructors
             * @{
             */

        Bar(int value) : value_(value)
        {}

    /*! @} */

private:
    int value_;
};
