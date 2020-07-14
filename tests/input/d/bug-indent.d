class TemplatedClass(T) {}
class TemplatedClass2(T,U) {}

class Axxxxxxxxxxxxxxxx {
alias A = int*;
void f(){
}
}

class C
{
 //--------------| <= (1) - non first col comment -> indent
Axxxxxxxxxxxxxxxx.A createAssignment()
{
	return(null);
}
void func2(Axxxxxxxxxxxxxxxx[] container){
	foreach (v; container) {
		v.f();
	}
}

 //                  | <= (2)
void func3(TemplatedClass!int aValue)
{
}

void func4(TemplatedClass2!(int, int) b){
}
}

int main(){
	return 0;
}
