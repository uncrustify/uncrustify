//It is applying double indentation
m_ApplicationView = AppC::ApplicationView::GetForCurrentView();
m_ViewConsolidateEvtToken = m_ApplicationView->Consolidated +=
    ref new AppC::TypedEventHandler<AppC::ApplicationView^, AppC::ApplicationViewConsolidatedEventArgs^>(this, &FrameworkView::InternalOnViewConsolidated);

m_WindowActivatedEvtToken = m_CoreWindow->Activated +=
    ref new AppC::TypedEventHandler<AppC::CoreWindow^, AppC::WindowActivatedEventArgs^>(this, &FrameworkView::InternalOnWindowActivated);

m_SizeChangedEvtToken = m_CoreWindow->SizeChanged +=
    ref new AppC::TypedEventHandler<AppC::CoreWindow^, AppC::WindowSizeChangedEventArgs^>(this, &FrameworkView::InternalOnWindowSizeChanged);

m_VisibilityChangedEvtToken = m_CoreWindow->VisibilityChanged +=
    ref new AppC::TypedEventHandler<AppC::CoreWindow^, AppC::VisibilityChangedEventArgs^>(this, &FrameworkView::InternalOnWindowVisibilityChanged);

m_WindowClosedEvtToken = m_CoreWindow->Closed +=
    ref new AppC::TypedEventHandler<AppC::CoreWindow^, AppC::CoreWindowEventArgs^>(this, &FrameworkView::InternalOnWindowClosed);
