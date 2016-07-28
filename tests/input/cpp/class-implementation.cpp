template<class T>
class TestTemp  
{
public:
    TestTemp();
    void SetValue( T obj_i );
    T Getalue();
private:
    T m_Obj;
};

template <class T>
TestTemp<T>::TestTemp()
{
}
template <class T>
void TestTemp<T>::SetValue( T obj_i )
{
}
 
template <class T>
T TestTemp<T>::Getalue()
{
    return m_Obj;
}
