struct MyClass : public Foo,
                 private Bar {
  MyClass(
      int a,
      int b,
      int c)
      : m_a(a),
        m_b(b),
        m_c(c) {}

 private:
  int m_a, m_b, m_c;
};

struct TheirClass
      : public Foo,
        private Bar {};
