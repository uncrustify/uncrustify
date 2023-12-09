void f()
{
	auto[x,y]=g();
	auto[  x,  y  ]  =  z;
	auto& [x,y]=g();
	auto  & [  x,  y  ]  =  z;
}
