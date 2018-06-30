void Foo::doo()
{
	m_stackCache[m_currentStackNr]->operator [](0) = new QStandardItem(QString::number(m_currentStackNr));
	m_stackCache[m_currentStackNr]->operator [](1) = new QStandardItem(tr("OK"));
	m_stackCache[m_currentStackNr]->operator [](2) = new QStandardItem("0");
	m_stackCache[m_currentStackNr]->operator [](3) = new QStandardItem("0");
	m_stackCache[m_currentStackNr]->operator [](4) = new QStandardItem();
}
