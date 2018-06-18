
BEGIN_MESSAGE_MAP(CUSB2_camera_developementDlg, CDialog)
    ON_COMMAND(IDC_ESCAPE, On_Escape)
    ON_COMMAND(IDC_8_BIT, On_8_Bit)
    ON_COMMAND(IDC_14_BIT, On_14_Bit)
    ON_COMMAND(IDC_ACQUIRE, On_Acquire)
    ON_COMMAND(IDC_SAVE_COLUMN_AVERAGES, On_Save_Column_Averages)
    ON_COMMAND(IDC_SAVE_ROW_AVERAGES, On_Save_Row_Averages)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

namespace one
{
    namespace two
    {
        int Func(int a,
                 int b)
        {
            return a + b;
        }
    }
}

using namespace one::two;

void Func2(int c,
           int d)
{
}

int main()
{
    int a;

    switch (a)
    {
        case 0:
            Func2(1, Func(1, 2));
            Func2(1, one::two::Func(1, 2));
            break;
    }
}
