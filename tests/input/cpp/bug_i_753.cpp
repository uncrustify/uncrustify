void Test::init()
{
    connect( m_ppcCom,
        SIGNAL( sigReceivedBundle(QString) ),
        SLOT( doProcessBundle(QString) ) );
    connect( m_ppcCom,
        SIGNAL( sigReceivedBundle          ),
        SLOT( doProcessBundle ) );
}
