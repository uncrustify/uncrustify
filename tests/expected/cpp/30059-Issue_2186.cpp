using namespace std;

namespace ui      { class CClass; }              // Expected to stay as-is
namespace ui::dlg { class CClassDlg; }           // Expected to stay as-is (new in C++17)

namespace ui                // Brace should be on the next line
{
	class CClass1;      // Should be indented
	class CClass2;
	class CClass3;
	class CClass4;
	class CClass5;
	class CClass6;
	class CClass7;
}
