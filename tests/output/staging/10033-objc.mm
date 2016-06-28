void foo()
{
    if (key)
        ret.key = [NSString stringWithCharacters: &key length: 1];

    [gUndoMenu->m_UndoItem setTitle: [NSString stringWithFormat: [NSString stringWithUTF8String: localizedUndo], undoName.c_str()]];
    [gUndoMenu->m_RedoItem setTitle: [NSString stringWithFormat: [NSString stringWithUTF8String: localizedRedo], redoName.c_str()]];
}
