#ifndef _FOO_BAR_H_INCLUDED_
#define _FOO_BAR_H_INCLUDED_

class CFooBarDlg : public CDialog
{
// Construction
public:
   CFooBarDlg(CFooBar *pDataMan,
              CWnd* pParent = NULL );
   virtual ~CFooBarDlg();

   void Initialize( BYTE nDelay=100 );

   UINT GetCount() { return (m_nCount); }

   void SetCount(int count=1)
   {
      if ((count > 0) && (count < MAX_COUNT))
      {
         m_nCount = count;
      }
   };

   // Dialog Data
   //{{AFX_DATA(CATCSMgrDlg)
   enum { IDD = IDD_ATCS_MGR_DLG };
   //}}AFX_DATA

protected:
   int   m_nCount;

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CATCSMgrDlg)
protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation

   // Generated message map functions
   //{{AFX_MSG(CATCSMgrDlg)
   virtual BOOL OnInitDialog();
   afx_msg void OnTimer(UINT nIDEvent);
   afx_msg void OnBtnSendFooBar();
   afx_msg void OnSelchangeFooBarCombo();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};

#endif   /* _FOO_BAR_H_INCLUDED_ */

