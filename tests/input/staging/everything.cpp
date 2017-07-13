extern "C" T* F();

void G()
{
	C x(p, y*z);

	auto weights[] = {dx*dy};
}

if (true)				// comment preceded by tabs
	return;

#if (WINVER < 0x0601)
#endif

	for (auto const& plugin : m_GfxNativePlugins)
		FindAndLoadUnityPlugin(plugin.c_str(), NULL);


char *DLLDetails[2][4]=
{
	{
		// 32 bit
		{"sce/vagconv2_32/vagconv2.dll"},
		{"_sceVagConvertInit@4"},{"_sceVagConvert@12"},{"_sceVagConvertFin@4"}
	},
	{
		// 64 bit
		{"sce/vagconv2_64/vagconv2.dll"},
		{"sceVagConvertInit"},{"sceVagConvert"},{"sceVagConvertFin"}
	}
};

void ScriptUpdatingManager::ForEachAssemblyWithObsoleteAPI (void (*func)(int))
{
	for_each(assembliesWithObsoleteAPI.begin(), assembliesWithObsoleteAPI.end(), func);
}

namespace
{
	template <size_t size>
	inline void FixDeviceName(LPCWSTR source, WCHAR (&destination)[size])
	{
		// hack. windows xp sometimes returns device name starting with \?? instead of \\?

		wcscpy_s(destination, source);

		if (0 == wcsncmp(destination, L"\\??", 3))
		{
			destination[1] = L'\\';
		}
	}
}


template<class C, class Func>
inline Func for_each (C& c, Func f)
{
}

MIDL_INTERFACE("973510db-7d7f-452b-8975-74a85828d354")
IFileDialogEvents: public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE OnFileOk(
		/* [in] */ __RPC__in_opt IFileDialog *pfd) = 0;
};

void ComputeTriangleTangentBasis (const Vector3f* vertices, const 	Vector2f* uvs, const UInt32* indices, TangentInfo out[3]);

// Runtime\Threads\ExtendedAtomicOps-clang-gcc.h
#	if (__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
#		error "__atomic built-in functions not supported on GCC versions older than 4.7"
#	endif

// Runtime\Utilities\File.h
inline bool GetFolderContentsAtPath (const std::string& pathName, std::set<std::string>& paths)
{
	__FAKEABLE_FUNCTION_OVERLOADED__(GetFolderContentsAtPath, (pathName, paths),
		bool(const std::string&, std::set<std::string>&));

	return GetFolderContentsAtPath (pathName, kEnumerateAll, paths);
}

// Runtime\VirtualFileSystem\ArchiveFileSystem\ArchiveStorageReader.cpp
ArchiveStorageReader::CachedBlock* ArchiveStorageReader::AcquireAndPrefillUnusedBlock(UInt32 blockInd, CacheResult* result)
{
	CachedBlock *foundBlock = NULL;

	{
		Mutex::AutoLock lock(m_CachedBlocksMutex);

		// Get the least recent unused block
		int minLRUTime = m_LastUseCachedBlockMarker;
		for (size_t i = 0; i < m_CachedBlocks.size(); ++i)
		{
			CachedBlock *block = m_CachedBlocks[i];

			if (!AtomicCompareExchange(&block->bufferInWrite, 1, 0))			// Write lock
			{
				continue;
			}

			bool canUseBlock = true;

			if (!AtomicCompareExchange(&block->bufferReaders, 0, 0))			// No readers
				canUseBlock = false;
			else if (foundBlock != NULL && minLRUTime <= block->lastUseTime)	// Better than current
				canUseBlock = false;
		}
	}
}

// Editor\Src\AssetPipeline\HEVAGTool.cpp
namespace psp2
{
namespace audioconverter
{
static HINSTANCE s_Dll = 0;
lpFunc1 ConvertInit = 0;
lpFunc2 Convert = 0;
lpFunc3 ConvertFin = 0;
char *DLLDetails[2][4]=
{
	{
		// 32 bit
		{"sce/vagconv2_32/vagconv2.dll"},
		{"_sceVagConvertInit@4"},{"_sceVagConvert@12"},{"_sceVagConvertFin@4"}
	},
	{
		// 64 bit
		{"sce/vagconv2_64/vagconv2.dll"},
		{"sceVagConvertInit"},{"sceVagConvert"},{"sceVagConvertFin"}
	}
};
}}

// Editor\Tools\UnityYAMLMerge\libUnityYAML\Src\YAMLOutput.cpp
YAMLDiffOutput& operator<<(YAMLDiffOutput& w, YAMLDiffOutput& (*pf)(YAMLDiffOutput&))
{
	return pf(w);
}

// Runtime\GfxDevice\d3d11\VertexDeclarationD3D11.cpp
static FORCE_INLINE DXGI_FORMAT GetD3D11VertexDeclType(const ChannelInfo& info)
{
	switch (info.format)
	{
	case kChannelFormatFloat:
		{
			switch (info.dimension)
			{
			case 1: return DXGI_FORMAT_R32_FLOAT;
			case 2: return DXGI_FORMAT_R32G32_FLOAT;
			case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
			case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			break;
		}
	case kChannelFormatFloat16:
		{
			switch (info.dimension)
			{
			case 2: return DXGI_FORMAT_R16G16_FLOAT;
			case 4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
			}
			break;
		}
	case kChannelFormatColor:
		{
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	case kChannelFormatByte:
		{
			return DXGI_FORMAT_R8G8B8A8_SNORM;
		}
	}
	Assert("No matching D3D11 vertex decl type!");
	return DXGI_FORMAT_UNKNOWN;
}

/// new

struct invoke_fptr<R(VKAPI_PTR*)(FArgs...)>
{
    
}

void foo()
{
#if defined(SUPPORT_FEATURE)
    bar();
#endif // SUPPORT_FEATURE
    // Handle error
    if (error != 0)
    {
    }
}

friend std::ostream& operator<<(std::ostream& os, const ScriptingObjectPtr& o);

extern "C" void __declspec(dllexport) GetAccountNameAndDomain(HWND /*hwndParent*/, int string_size, TCHAR * variables, stack_t** stacktop, extra_parameters* /*extra*/)
{
}
