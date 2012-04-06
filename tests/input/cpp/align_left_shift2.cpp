void g()
{
    RLOGD(m_log)
        << "str1"
        << var;

    if (something)
        cout << "blah";

}

void f()
{
    cout << something(
        arg);
    cout
        << "something";
    cout <<
        "something";

    RLOGD(m_log)
        << "WriteReqSize()";

    RLOGD(m_log) <<
        base::sprintfT(
            "something %u ",
            m_pendingAccepts);

    RLOGDD(m_log) << sprintfT(
        "something id=%u",
        newSocket->GetId());
}
