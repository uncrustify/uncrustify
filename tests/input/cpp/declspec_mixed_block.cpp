#define EXPORT __declspec(dllexport)
extern "C" {
	EXPORT DWORD NvOptimusEnablement1 = 1;
	EXPORT DWORD NvOptimusEnablement2 = 3;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance1 = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance2 = 2;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance3 = 3;
}
