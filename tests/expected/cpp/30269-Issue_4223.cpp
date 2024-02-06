template < class... Args >
void
throw_error( Args... args )
{
  std::stringstream ss;
  bool dummy[] = { ( ss << args ).good()... };
  ( void ) dummy;
  throw std::runtime_error( ss.str() );
}
