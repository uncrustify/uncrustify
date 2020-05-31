class CDiagnostic
{
        CDiagnostic& operator<<( int value_ ) { return ns::operator<<( *this, value_ ); }
};
