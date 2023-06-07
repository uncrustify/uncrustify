class foo {
public:
    int var;
 
    foo(int x) { var = x; }
};

int main() 
{
    int a = 2;
    a++;

    foo f(3);
    f.var++;
}

InternalFunctionImp::InternalFunctionImp(ExecState *exec)
  : ObjectImp(static_cast<FunctionPrototypeImp*>(exec->interpreter()->builtinFunctionPrototype().imp()))
{
  for (int z=0; z<count; z++)
  {
    TQString hlName = KateFactory::self()->schemaManager()->list().operator[](z);

    if (names.contains(hlName) < 1)
    {
      names << hlName;
      popupMenu()->insertItem ( hlName, this, TQT_SLOT(setSchema(int)), 0,  z+1);
    }
  }

  struct sockaddr_in *sin1 = (sockaddr_in *) s1.address();
  struct sockaddr_in *sin2 = (sockaddr_in *) s2.address();

  if ((bytesLeft < 0 ) || (len > (uint) bytesLeft))
  {
     tqWarning("[dcopserver] Corrupt data!");
     printf("[dcopserver] bytesLeft: %d, len: %d", bytesLeft, len);
     return result;
  }
  result.TQByteArray::resize( (uint)len );

	if (pbar->isVisible() && pbar->isEnabled() &&
		pbar->progress() != pbar->totalSteps())
	{
		++iter.data();
		if (iter.data() == 28)
			iter.data() = 0;
		iter.key()->update();
	}
}
