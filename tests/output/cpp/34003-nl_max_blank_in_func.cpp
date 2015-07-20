//regular function
void func0()
{
	return;
}
// ========================================================================
//member function
void cls::func1()
{
	return;
}
// ========================================================================
//lambda function
const auto l = [](){
		       return 1;
	       };
// ========================================================================
//regular function in class
class cls
{
public:




void func0()
{
	return;
}




}
// ========================================================================
//member function in class

// ========================================================================
//lambda function in class
class cls
{
pubic:




const auto l = [](){
		       return 1;
	       };




}
// ========================================================================
//regular function in class in namespace
namespace ns
{




class cls
{
public:




void func0()
{
	return;
}




}




}
// ========================================================================
//member function in class in namespace

// ========================================================================
//lambda function in class in namespace
namespace ns
{




class cls
{
pubic:




const auto l = [](){
		       return 1;
	       };




}




}
