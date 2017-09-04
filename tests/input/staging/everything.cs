void F()
{
	Editor.CodeEval.Eval(() =>
	{
		PrefabUtility.CreatePrefab(assetFolderPath + "/Prefab.prefab", Testing.CreatePrimitive(PrimitiveType.Cube));
		AssetBundleBuild[] buildInfo =
		{
			new AssetBundleBuild{	assetBundleName = assetBundleOutputFolder, 
									assetNames = new[] { assetFolderPath + "/Prefab.prefab"}  
								}
		};
	});
}

if (m_Lazy) dataSource = new LazyTestDataSource(m_TreeView, m_BackendData);
else		dataSource = new TestDataSource(m_TreeView, m_BackendData);

if (expand)		oldExpandedSet.UnionWith (candidates);
else			oldExpandedSet.ExceptWith (candidates);

//:original
public class Container { public float prop { get; set; } };

//:new
public class Container { public int prop { get; set; } };

public static AndroidDevice FindDevice(string id)
{
	Console.WriteLine("AndroidDevice - Probing for devices...");
	AssertAnyDeviceReady();
	try { return GetDeviceById(id); } catch (Exception) {}
	try { return GetDeviceByProductId(id); } catch (Exception) {}
	throw new ApplicationException("No suitible device found: " + id);
}

namespace UnityEditor
{
	[System.Serializable]
	internal class AvatarMappingEditor : AvatarSubEditor
	{
		internal class Styles
		{
		protected int[][] m_BodyPartHumanBone =
		{
			new int[]
			{
				 (int)HumanBodyBones.LeftThumbProximal , (int)HumanBodyBones.LeftThumbIntermediate, (int)HumanBodyBones.LeftThumbDistal,
				 (int)HumanBodyBones.LeftIndexProximal , (int)HumanBodyBones.LeftIndexIntermediate, (int)
			}
		}
		}

				protected int[][] m_Muscles = {
									new int[] { (int)DoF.BodyDoFStart + (int)BodyDoF.SpineFrontBack,
										(int)DoF.BodyDoFStart + (int)BodyDoF.SpineLeftRight
									},
									new int[] {
											(int)DoF.LeftThumbDoFStart + (int)FingerDoF.ProximalDownUp,
											(int)DoF.LeftThumbDoFStart + (int)FingerDoF.ProximalInOut
									},
				}
	}
}


public static AndroidDevice FindDevice(string id)
{
	Console.WriteLine("AndroidDevice - Probing for devices...");
	AssertAnyDeviceReady();
	try { return GetDeviceById(id); } catch (Exception) {}
	try { return GetDeviceByProductId(id); } catch (Exception) {}
	throw new ApplicationException("No suitible device found: " + id);
}

internal static AndroidJavaObject AndroidJavaObjectDeleteLocalRef(IntPtr jobject)
{
	try { return new AndroidJavaObject(jobject); } finally { AndroidJNISafe.DeleteLocalRef(jobject); }
}

extern "C"
{
void* APK_FileOpen(const char* path)
{
}
}

for (auto const& plugin : m_GfxNativePlugins)
    FindAndLoadUnityPlugin(plugin.c_str(), NULL);

namespace NS
{
	class C
	{
		public static uint CreateAssetBundleFromPrefab(UnityEditorProcess unityEditorProcess, string prefabName, string assetBundleFile, BuildAssetBundleOptions buildAssetBundleOptions, BuildTarget buildTarget)
		{
			unityEditorProcess.Log.AssertHasNoWaitingMessages();
			assetBundleFile = assetBundleFile.Replace("\\", "/");

			if (!assetBundleFile.Contains(".unity3d"))
				throw new ArgumentException("The asset bundle filename should end with .unity3d");

			unityEditorProcess.CodeEval.Eval((name, bundleFile, assetBundleOptions, target) =>
			{
				var prefab = AssetDatabase.LoadMainAssetAtPath("Assets/" + name + ".prefab");
				uint crc;

				if(prefab == null)
					UnityEngine.Debug.Log("The prefab you tried to load is null");
#pragma warning disable 618
				else if(!BuildPipeline.BuildAssetBundle(prefab, null, bundleFile, out crc, assetBundleOptions, target))
#pragma warning restore 618
					UnityEngine.Debug.Log("Couldn't Create the Prefab AssetBundle");
				else
					UnityEngine.Debug.Log("CRC: " + crc);
			}, prefabName, assetBundleFile, buildAssetBundleOptions, buildTarget);
		}
	}
}

/// new

private void Foo()
{
    Action<UnityPlayerBaseStartInfo, GraphicsTestRunConfiguration, Action<string, Bitmap, long>, RenderingBackend, DX11FeatureLevel ? , string> playerRunnerImageCallback
                 = (playerStartInfo, description, incomingScreenshotCallback, configuration, dx11Featurelevel, graphicsDriverType) => incomingScreenshotCallback(filename, new Bitmap(1, 1), 42);
}
