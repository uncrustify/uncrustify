static void GPUFailedMsgA(const long long int error, const char* file, int line)
{
	if (GPUFailedMsgAI(error, file, line)) throw std::runtime_error("Failure");
}
