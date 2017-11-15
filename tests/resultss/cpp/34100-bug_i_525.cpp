EXEC SQL BEGIN DECLARE SECTION;
static char    *tbuf;
EXEC SQL END DECLARE SECTION;

void myfunc1()
{
	exec sql execute immediate :tbuf;
}

void myfunc2()
{
	EXEC SQL EXECUTE IMMEDIATE :tbuf;
}
