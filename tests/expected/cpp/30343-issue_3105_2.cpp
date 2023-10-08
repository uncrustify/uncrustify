struct KPluginSelectionWidget::KPluginSelectionWidgetPrivate
{
	KPluginSelectionWidgetPrivate(KPluginSelector *_kps, const TQString &_cat,
	        TDEConfigGroup *_config) :
			widgetstack(0), kps(_kps), config(_config), tooltip(0), catname(_cat), currentplugininfo(0),
			        visible(true), currentchecked(false), changed(0)
	{
		moduleParentComponents.setAutoDelete(true);
	}
};

KPasswordDialog::KPasswordDialog(Types type, bool enableKeep, int extraBttn, TQWidget *parent,
        const char *name) :
		KDialogBase(parent, name, true, "", Ok | Cancel | extraBttn,
		        Ok, true), m_Keep(enableKeep ? 1 : 0), m_Type(type), m_keepWarnLbl(0),
		        d(new KPasswordDialogPrivate)
{
	d->iconName = "password";
	init();

	const TQString strengthBarWhatsThis(i18n(
	        "The password strength meter gives an indication of the security "
	        "of the password you have entered.  To improve the strength of "
	        "the password, try:\n"
	        " - using a longer password;\n"
	        " - using a mixture of upper- and lower-case letters;\n"
	        " - using numbers or symbols, such as #, as well as letters."));

	int retVal = KMessageBox::warningContinueCancel(this,
	        i18n("The password you have entered has a low strength. "
	             "To improve the strength of "
	             "the password, try:\n"
	             " - using a longer password;\n"
	             " - using a mixture of upper- and lower-case letters;\n"
	             " - using numbers or symbols as well as letters.\n"
	             "\n"
	             "Would you like to use this password anyway?"),
	        i18n("Low Password Strength"));
}

static const int POPUP_FLAGS = TQt::WStyle_Customize | TQt::WDestructiveClose | TQt::WX11BypassWM |
        TQt::WStyle_StaysOnTop | TQt::WStyle_Tool | TQt::WStyle_NoBorder;

KPassivePopup::KPassivePopup(TQWidget *parent, const char *name, WFlags f) :
		TQFrame(0, name, (WFlags)(f ? (int)f : POPUP_FLAGS)), window(parent ? parent->winId() : 0L),
		        msgView(0), topLayout(0), hideDelay(DEFAULT_POPUP_TIME),
		        hideTimer(new TQTimer(this, "hide_timer")), m_autoDelete(false)
{
	init(DEFAULT_POPUP_TYPE);

	move(right ? d->anchor.x() - width() + 20 : (d->anchor.x() < 11 ? 11 : d->anchor.x() - 20),
	        bottom ? d->anchor.y() - height() : (d->anchor.y() < 11 ? 11 : d->anchor.y()));
}

TDEToggleAction* showMenubar(const TQObject *recvr, const char *slot, TDEActionCollection *parent,
        const char *_name)
{
	TDEToggleAction *ret;
	ret = new TDEToggleAction(i18n("Show &Menubar"), "showmenu",
	        TDEStdAccel::shortcut(TDEStdAccel::ShowMenubar), recvr, slot, parent,
	        _name ? _name : name(ShowMenubar));
	ret->setWhatsThis(i18n("Show Menubar<p>"
	                       "Shows the menubar again after it has been hidden"));
	KGuiItem guiItem(i18n("Hide &Menubar"), 0 /*same icon*/, TQString::null,
	        i18n("Hide Menubar<p>"
	             "Hide the menubar. You can usually get it back using the right mouse button inside the window itself."));
	return ret;
}

KProgressBoxDialog::KProgressBoxDialog(TQWidget *parent, const char *name, const TQString &caption,
        const TQString &text, bool modal) :
		KDialogBase(KDialogBase::Plain, caption, KDialogBase::Cancel,
		        KDialogBase::Cancel, parent, name, modal), mAutoClose(true), mAutoReset(false),
		        mCancelled(false), mAllowCancel(true), mAllowTextEdit(false), mShown(false),
		        mMinDuration(2000), d(new KProgressBoxDialogPrivate)
{
	KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());
}

TDEPopupMenu* KPixmapRegionSelectorWidget::createPopupMenu()
{
	TDEPopupMenu *popup = new TDEPopupMenu(this, "PixmapRegionSelectorPopup");
	popup->insertTitle(i18n("Image Operations"));

	TDEAction *action = new TDEAction(i18n("&Rotate Clockwise"), "object-rotate-right", 0,
	        TQT_TQOBJECT(this), TQT_SLOT(rotateClockwise()), TQT_TQOBJECT(popup), "rotateclockwise");
	action->plug(popup);

	action = new TDEAction(i18n("Rotate &Counterclockwise"), "object-rotate-left", 0,
	        TQT_TQOBJECT(this), TQT_SLOT(rotateCounterclockwise()), TQT_TQOBJECT(popup),
	        "rotatecounterclockwise");
	action->plug(popup);
	return popup;
}
