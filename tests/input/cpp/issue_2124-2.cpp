if(x)[[likely]]{}
if(x)
[[unlikely]]
{}

g();

if(x)[[likely]]l();
if(x)
[[unlikely]]
l();

g();

if(x)
[[unlikely]]
l1();
else
l2();

g();

if(x)
#if __has_cpp_attribute(likely)
[[likely]]
#endif
	return false;
else
	return true;

g();

while(true)[[likely]]{break;}
while(true)
[[unlikely]]
{break;}

g();

if(x)
	[[likely]]
{
	if(y)
		[[likely]]
	{}
}

g();