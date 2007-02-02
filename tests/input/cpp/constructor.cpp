
IMPLEMENT_DYNAMIC(CPropertiesDlg, CDialog)
CPropertiesDlg::CPropertiesDlg( CPtcMsgSimControlModule *pcmPtcMsg,
                                CWnd* pParent /*=NULL*/):
   CDialog( CPropertiesDlg::IDD, pParent ),
   m_pspRouter( pcmPtcMsg ),
   m_pspScm( pcmPtcMsg )
{
   m_pcmPtcMsg = pcmPtcMsg;
}

CPropertiesDlg::~CPropertiesDlg()
{
}

void CPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

CFooBar::CFooBar(CWnd* pParent /*=NULL*/)
	: CDialog(CFooBar::IDD, pParent),
   m_parent(pParent)
{
	//{{AFX_DATA_INIT(CRouterBrowser)
	//}}AFX_DATA_INIT

   m_nFoo = 0;
   m_nBar = 0;
}

