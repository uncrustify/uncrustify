KJS::Value KJS::KateJSViewProtoFunc::call(KJS::ExecState *exec, KJS::Object &thisObj, const KJS::List &args)
{
  switch (id)
  {
    case KateJSView::SetCursorPositionReal:
      return KJS::Boolean( view->setCursorPositionReal( args[0].toUInt32(exec), args[1].toUInt32(exec) ) );

    // SelectionInterface goes in the view, in anticipation of the future
    case KateJSView::Selection:
      return KJS::String( view->selection() );
  }

  return KJS::Undefined();
}

void KateXmlIndent::getLineInfo (uint line, uint &prevIndent, int &numTags,
                                 uint &attrCol, bool &unclosedTag)
{
  for(pos = 0; pos < len; ++pos) {
    int ch = text.at(pos).unicode();
    switch(ch) {
      case '<':
        ++numTags;
        break;

      // don't indent because of DOCTYPE, comment, CDATA, etc.
      case '!':
        if(lastCh == '<') --numTags;
        break;

      // don't indent because of xml decl or PI
      case '?':
        if(lastCh == '<') --numTags;
        break;
    }
  }
}

static YYSIZE_T yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
  {
    for (;;)
      switch (*++yyp)
      {
        case '\\':
          if (*++yyp != '\\')
            yyres[yyn] = *yyp;
        /* Fall through.  */
        default:
          if (yyres)
            yyres[yyn] = *yyp;
          yyn++;
          break;
      }
  }
  return yystpcpy (yyres, yystr) - yyres;
}

Value RegExpProtoFuncImp::call(ExecState *exec, Object &thisObj, const List &args)
{
  if (!thisObj.inherits(&RegExpImp::info)) {
    if (thisObj.inherits(&RegExpPrototypeImp::info)) {
      switch (id) {
        case ToString: return String("//"); // FireFox returns /(?:)/
      }
    }
    return err;
  }
}
