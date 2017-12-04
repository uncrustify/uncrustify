typename std::enable_if<!std::is_void<T>::value, QVector<T> >::type dummy(const std::function<T*(const S&)>& pFunc, const QVector<S>& pItems)
{
	return QVector<T>();
}


typename std::enable_if<!std::is_void<T>::value, QVector<T> >::type filter(const std::function<bool(const T&)>& pFunc, const QVector<T>& pItems)
{
	return QVector<T>();
}
