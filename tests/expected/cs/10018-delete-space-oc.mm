/* EditorApplication */

#include <string>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>

class HierarchyState;
@interface EditorApplication: NSObject
{
	IBOutlet id m_MainWindow;
	IBOutlet id m_PaneController;
	id m_RenderTimer;

	IBOutlet id m_CutItem;
	IBOutlet id m_CopyItem;
	IBOutlet id m_PasteItem;

	IBOutlet id m_DuplicateItem;
	IBOutlet id m_DeleteItem;

	IBOutlet id m_FrameSelectedItem;
	IBOutlet id m_FindItem;
	IBOutlet id m_SelectAllItem;
}

-(IBAction)SaveAssets: (id)sender;
-(IBAction)CloseScene: (id)sender;
-(IBAction)NewProject: (id)sender;
-(IBAction)OpenProject: (id)sender;

-(IBAction)SaveAsSceneToDisk: (id)sender;

-(IBAction)EnterSerialNumber: (id)sender;
-(IBAction)ReturnLicense: (id)sender;
-(IBAction)CompileScene: (id)sender;
-(IBAction)CompileSceneAutomatic: (id)sender;

-(IBAction)saveDocument: (id)sender;

-(IBAction)LoadSceneFromDisk: (id)sender;

-(void)RemoveDisplayTimer;
-(void)RegisterUpdateTimer: (int)frequency;

-(void)refreshModifiedFile: (NSAppleEventDescriptor*)event withReplyEvent: (NSAppleEventDescriptor*)replyEvent;
-(void)closeFile: (NSAppleEventDescriptor*)event withReplyEvent: (NSAppleEventDescriptor*)replyEvent;

-(IBAction)ShowAboutDialog: (id)sender;
-(IBAction)ShowPreferences: (id)sender;
-(IBAction)ShowPackageManager: (id)sender;

-(IBAction) delete: (id)sender;
-(IBAction) copy: (id)action;
-(IBAction)paste: (id)action;
-(IBAction)duplicate: (id)action;
-(IBAction)cut: (id)action;
-(IBAction)selectAll: (id)action;
-(IBAction)find: (id)action;
-(IBAction)frameSelected: (id)action;
-(IBAction)frameSelectedWithLock: (id)action;

// Assetstore protocol handler and registration
-(void)registerAssetStoreURLProtocol;
-(void)getUrl: (NSAppleEventDescriptor*)event withReplyEvent: (NSAppleEventDescriptor*)replyEvent;

@end

#endif
