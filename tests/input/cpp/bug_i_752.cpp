int a()
{
    for(QStringList::const_iterator codesIt = _codes.constBegin(); codesIt != _codes.constEnd(); ++codesIt) {
        if(     // Current codes enough to compare:
                ( ( *codesIt ).size() <= strId ) ||
                // Character on this slot was not readable:
                ( ( *codesIt ).at( strId ) == m_wildcard ) ||
                // This character is matching:
                ( code.at( strId ) == ( *codesIt ).at( strId ) ) ) {
            // Ignore this slot:
            continue;
            }
    }
}
