void DCOPServer::processMessage( IceConn iceConn, int opcode)
{
    if ( !conn ) {
	tqWarning("[dcopserver] DCOPServer::processMessage message from unknown connection. [opcode = %d]", opcode);
	return;
    }
}
