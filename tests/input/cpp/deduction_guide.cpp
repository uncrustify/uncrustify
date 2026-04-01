template<typename T>
MyClass(T)->MyClass<T>;

template<typename T, typename U>
MyClass(T, U)->MyClass<std::pair<T, U>>;

template<typename K, typename V>
map(std::initializer_list<std::pair<K, V>>)->map<K, V>;

template<typename T>
explicit MyClass(T)->MyClass<T>;
