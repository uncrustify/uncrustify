//Test cases from diff-48
//Case 1
if (material.GetTextures().size() > 0)
{
    generator.Feed((int)material.GetTextures()[0]->GetDataFormat());
    generator.Feed((int)material.GetTextures()[0]->GetDataFormat());
}

//Case 2
int fun_name()
{
    return
        android::provider::Settings_System::GetInt(GetContentResolver(),
        android::provider::Settings_System::fSCREEN_OFF_TIMEOUT(),
        15000) / 1000;
}

//Case 3
int fun_name()
{
    UnityPlayer::AppCallbacks::Instance->InvokeOnUIThread(ref new UnityPlayer::AppCallbackItem([&]
    {
        using namespace Windows::UI::Popups;
        MessageDialog^ dialog = ref new MessageDialog(ConvertUtf8ToString(errorMessages), L"Fatal error");
        UICommand^ exitCommand = ref new UICommand(L"Exit", ref new UICommandInvokedHandler([](IUICommand^ cmd)
        {
            Windows::ApplicationModel::Core::CoreApplication::Exit();
        }));
        dialog->Commands->Append(exitCommand);
        dialog->ShowAsync();
    }), false);
}

//Case 4
int fun_name()
{
    if (mimeType == nullptr)
        return false;
    UnityPlayer::AppCallbacks::Instance->InvokeOnUIThread(ref new UnityPlayer::AppCallbackItem(
        [container, uri, stream, controlMode, scalingMode, completedEvent, &success] {
            auto mediaElement = ref new Xaml::Controls::MediaElement();
            container->Children->Append(mediaElement);
            if (uri != nullptr)
                mediaElement->Source = ref new Windows::Foundation::Uri(uri);
            else
                mediaElement->SetSource(stream, ref new Platform::String(L"video/mp4"));
            bool subscribeToKeyEvents = false;
            switch (controlMode)
            {
                case kMovieControlFull:
                case kMovieControlMinimal:
                    mediaElement->AreTransportControlsEnabled = true;
                    // MediaPlayer doesn't have a stop button, so at least allow to stop the video using key buttons
                    subscribeToKeyEvents = true;
                    break;
                case kMovieControlCancelOnInput:
                    {
                        subscribeToKeyEvents = true;
                        mediaElement->DoubleTapped += ref new Xaml::Input::DoubleTappedEventHandler(
                            [completedEvent, &success](Platform::Object^ sender, Xaml::Input::DoubleTappedRoutedEventArgs^) { CancelPlayback(sender, completedEvent, success); });
                        auto pointerHandler = ref new Xaml::Input::PointerEventHandler(
                            [completedEvent, &success](Platform::Object^ sender, Xaml::Input::PointerRoutedEventArgs^) { CancelPlayback(sender, completedEvent, success); });
                        mediaElement->PointerPressed += pointerHandler;
                        mediaElement->PointerWheelChanged += pointerHandler;
                        mediaElement->RightTapped += ref new Xaml::Input::RightTappedEventHandler(
                            [completedEvent, &success](Platform::Object^ sender, Xaml::Input::RightTappedRoutedEventArgs^) { CancelPlayback(sender, completedEvent, success); });
                        mediaElement->Tapped += ref new Xaml::Input::TappedEventHandler(
                            [completedEvent, &success](Platform::Object^ sender, Xaml::Input::TappedRoutedEventArgs^) { CancelPlayback(sender, completedEvent, success); });
                    }
                // continue
                case kMovieControlHidden:
                    mediaElement->AreTransportControlsEnabled = false;
                    break;
            }
            // Case 803250: Because there's no Stop button in MediaPlayer provide user with means to stop the video
            if (subscribeToKeyEvents)
            {
                // Note: We can't subscribe to MediaElement::KeyDown, because MediaElement is not a focusable element, so the event will never be received
                //       Interestingly on C# side there's no MediaElement::KeyDown
                s_KeyDownEvtToken = UnityPlayer::AppCallbacks::Instance->GetCoreWindow()->KeyDown +=
                    ref new MediaPlayer::TypedEventHandler<MediaPlayer::CoreWindow^, MediaPlayer::KeyEventArgs^>([mediaElement, completedEvent, &success](MediaPlayer::CoreWindow^, MediaPlayer::KeyEventArgs^ args)
                    {
                        if (args->VirtualKey == MediaPlayer::VirtualKey::Escape)
                        {
                            CancelPlayback(mediaElement, completedEvent, success);
                            args->Handled = true;
                        }
                        else
                        {
                            args->Handled = false;
                        }
                    });

                s_HardwareBackButtonPressedToken = MediaPlayer::SystemNavigationManager::GetForCurrentView()->BackRequested +=
                    ref new MediaPlayer::EventHandler<MediaPlayer::BackRequestedEventArgs^>([mediaElement, completedEvent, &success](Platform::Object^ sender, MediaPlayer::BackRequestedEventArgs^ args)
                    {
                        CancelPlayback(mediaElement, completedEvent, success);
                        args->Handled = true;
                    });
            }


            switch (scalingMode)
            {
                case kMovieScalingNone:
                    mediaElement->Stretch = Xaml::Media::Stretch::None;
                    break;
                case kMovieScalingAspectFit:
                    mediaElement->Stretch = Xaml::Media::Stretch::Uniform;
                    break;
                case kMovieScalingAspectFill:
                    mediaElement->Stretch = Xaml::Media::Stretch::UniformToFill;
                    break;
                case kMovieScalingFill:
                    mediaElement->Stretch = Xaml::Media::Stretch::Fill;
                    break;
            }
            mediaElement->IsFullWindow = true;
            mediaElement->MediaEnded += ref new Xaml::RoutedEventHandler([completedEvent, &success] (Platform::Object^ sender, Xaml::RoutedEventArgs^ args) {
                success = true;
                MoviePlaybackEnded(sender, completedEvent);
            });
            mediaElement->MediaFailed += ref new Xaml::ExceptionRoutedEventHandler([completedEvent, &success] (Platform::Object^ sender, Xaml::ExceptionRoutedEventArgs^) {
                success = false;
                MoviePlaybackEnded(sender, completedEvent);
            });
            mediaElement->Play();
        }), false);

    WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
    return success;
}

//Case 5
int func_name()
{
    (*s_Tiles)[tileId] = newTile;
    UnityPlayer::AppCallbacks::Instance->InvokeOnUIThread(
        ref new UnityPlayer::AppCallbackItem([newTile, tile, CreateSecondaryTile]
        {
            try
            {
                CreateSecondaryTile(tile)->Completed = ref new MTiles::AsyncOperationCompletedHandler<bool>(
                    [newTile, tile](MTiles::IAsyncOperation<bool>^ op, Windows::Foundation::AsyncStatus status)
                    {
                        if (status == Windows::Foundation::AsyncStatus::Completed && op->GetResults())
                        {
                            newTile->SetTileUpdaters(MTiles::TileUpdateManager::CreateTileUpdaterForSecondaryTile(tile->TileId),
                                MTiles::BadgeUpdateManager::CreateBadgeUpdaterForSecondaryTile(tile->TileId));
                        }
                        else
                        {
                            Mutex::AutoLock lock(s_Mutex);
                            s_Tiles->erase(newTile->GetId());
                        }
                    });
            }
            catch (...)
            {
                Mutex::AutoLock lock(s_Mutex);
                s_Tiles->erase(newTile->GetId());
            }
        }), false);

    return newTile;
}

//Case 6
bool VideoSwitch::Impl::GetAudioPresentationTimeUs(int64_t& presentationTimeUs) {}

//Case 7
bool VideoSwitch::Impl::GetAudioPresentationTimeUs(int64_t* presentationTimeUs) {}

//Case 8
bool VideoSwitch::Impl::read_input_data_from_extractor_send_to_decoder(int index, std::unique_ptr<movie::Decoder>& decoder) {}

//Case 9
MessageIdentifier::MessageIdentifier(const char* name, Options opts, const Unity::Type* type, const char* scriptParamName)
{
    Assert(name != NULL);
    AssertMsg(gRegisteredMessageIdentifiers == NULL || (*gRegisteredMessageIdentifiers)[0]->messageID == -1,
        "Defining variables of type MessageIdenfier is not allowed after InitializeEngine() has been called");
}

//Case 10
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY)(
    REFIID ridd,
    void** ppFactory
    ); // Used in CreateDXGIFactory

//Case11
Class abc
{
    parameterType = parameters[0].ParameterType;
    if (!(parameterType == typeof(string) ||
          parameterType == typeof(float) ||

          parameterType == typeof(int) ||
          parameterType == typeof(AnimationEvent) ||
          parameterType == typeof(UnityEngine.Object) ||
          parameterType.IsSubclassOf(typeof(UnityEngine.Object)) ||
          parameterType.IsEnum))
        continue;
};

//Case12
int func_name()
{
    if (go != NULL && (info == NULL ||
                       ((info->hierarchyDropMode & kDropUpon) ||
                        info->dragIntoWindowTarget == DragAndDropForwarding::kDragIntoSceneWindow)))
    {
        core::string actionName = tr("Assign VideoClip");
    }
}

//Case 13
int func_name()
{
    errbound = iccerrboundC * permanent + resulterrbound * Absolute(det);
    det += ((adx * adx + ady * ady) * ((bdx * cdytail + cdy * bdxtail)
        - (bdy * cdxtail + cdx * bdytail))
        + 2.0 * (adx * adxtail + ady * adytail) * (bdx * cdy - bdy * cdx))
        + ((bdx * bdx + bdy * bdy) * ((cdx * adytail + ady * cdxtail)
            - (cdy * adxtail + adx * cdytail))
            + 2.0 * (bdx * bdxtail + bdy * bdytail) * (cdx * ady - cdy * adx))
        + ((cdx * cdx + cdy * cdy) * ((adx * bdytail + bdy * adxtail)
            - (ady * bdxtail + bdx * adytail))
            + 2.0 * (cdx * cdxtail + cdy * cdytail) * (adx * bdy - ady * bdx));
    if ((det >= errbound) || (-det >= errbound))
    {
        return det;
    }
}

//Case 14
int func_name()
{
    core::string gen3 = gen2;
    gen2 += GenerateShaderForInputs(gen2, params.o.mode, pass,
        params.p.entryName[kProgramSurface],
        params.o.finalColorModifier, params.o.finalPrepassModifier, params.o.finalGBufferModifier,
        *lightModel,
        lightNeedsViewDir,
        funcData.desc, true);
}

//Case 15
Class abc{
    return  (metrics.meanAngularDistortion < angleThreshold
        && metrics.meanAreaDistortion < areaThreshold
        );
};

//Case 16
public class ProductDefinition
{
    public ProductDefinition(string id, string storeSpecificId, ProductType type, bool enabled, PayoutDefinition payout) : this(id, storeSpecificId, type, enabled, new List<PayoutDefinition> { payout }
        )
}
//Case 17
int func_name() {
    if ((Vars.implicitArgument1.Dereference().IndexedBy(1) &&
         Vars.implicitArgument1.Dereference().IndexedBy(1).JamEquals(Vars.implicitArgument2.Dereference().IndexedBy(1))))
    {
        GlobalVariables.Singleton.DereferenceElementsNonFlat(Vars.implicitArgument1).Assign(Vars.implicitArgument1.Dereference().IndexedBy("2-"));
        GlobalVariables.Singleton.DereferenceElementsNonFlat(Vars.implicitArgument2).Assign(Vars.implicitArgument2.Dereference().IndexedBy("2-"));
    }

//Case 18
    var gView = selectionContainer as GraphView;

    if (gView != null &&
        (parent == gView.contentViewContainer ||
         (parent != null && parent.parent == gView.contentViewContainer)))
    {
        return 0;
    }

//Case 19
    try
    {
        return (s_AvailableKeyalias = AndroidSDKTools.GetInstanceOrThrowException().
                ReadAvailableKeys(keystore, storepass));
    }


//Case 20
    int number_to_move = (yy_n_chars) + 2;
    char *dest = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[
        YY_CURRENT_BUFFER_LVALUE->yy_buf_size + 2];


//Case 21
    for (int x = offset.x; x + size.x <= image.GetWidth(); x += size.x)
    {
        Rectf* rect = new Rectf(
            x,
            imageHeight - y - size.y,
            size.x,
            size.y
            );
    }


//Case 22
    core::string text = Format(tr("The project at %s contains inconsistencies in the asset database.\n"
        "You must reimport the project in order to get the asset database into a consistent state.\n"
        "Depending on project size, rebuilding the project may take a while complete.\n"), projectPath.c_str());

//Case 23
    {
        if (colorImage == NULL)
            colorImage = vpx_img_alloc(
                NULL, VPX_IMG_FMT_I420, GetAdjustedWidth(), GetAdjustedHeight(), 16);
        return colorImage;
    }

//Case 24
    if (!IsValidChannel(fieldName.c_str()))
    {
        core::string msg = Format("Vertex program '%s': missing/unknown input semantic for %s: %s",
            programName.c_str(), name.c_str(), semantic.c_str());
        outErrors.AddImportError(msg.c_str(), startLine, false, kShaderCompPlatformPS4);
        //printf("[ERR] %s, %d\n", msg.c_str(), startLine);
    }
}
//Case 25
static FORCEINLINE void* win32direct_mmap(size_t size)
{
    void* ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
        PAGE_READWRITE);
    return (ptr != 0) ? ptr : MFAIL;
}

//Case 26
void *tfBufferPtr = sce::Gnm::getTessellationFactorRingBufferBaseAddress();
int func_name()
{
    if (!ret)
    {
        ret = sceKernelMapDirectMemory(&tfBufferPtr,
            sce::Gnm::kTfRingSizeInBytes,
            SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE | SCE_KERNEL_PROT_GPU_ALL,
            0, //flags
            sm_tOffset,
            tfAlignment);
    }

//Case 27
    {
        TangoClientPlugin &tangoClientPlugin = GetTangoClientPlugin();
        TangoExternal::TangoErrorType err = tangoClientPlugin.SetCallback_OnTextureAvailable(
            TangoExternal::TangoCameraId::TANGO_CAMERA_COLOR,
            nullptr,
            &OnTextureAvailableRouter);
        return (err == TangoExternal::TANGO_SUCCESS);
    }
}

//Case 28
void MainWindow::on_attachFileButton_clicked()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Attach Files"));
}
