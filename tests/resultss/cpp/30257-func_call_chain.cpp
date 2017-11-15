void f()
{
    m_complete.back().m_replicas.clear();

    m_complete.back().m_replicas.push_back(serverId);
    m_pending.front().m_replicas.erase(r);
}
