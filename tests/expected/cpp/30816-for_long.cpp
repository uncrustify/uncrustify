void
foo()
{
  for (std::map<std::string, std::string>::iterator it =
         m_stat_http_conn_total.m_stat_response_codes.begin();
       it != m_stat_http_conn_total.m_stat_response_codes.end();
       ++i)
  {
    bar(it);
  }
}

