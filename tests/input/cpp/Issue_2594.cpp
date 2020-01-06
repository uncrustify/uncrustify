int GPUReconstructionOCL2Backend::GetOCLPrograms()
{

#ifdef OPENCL2_ENABLED_SPIRV // clang-format off
  if (ver >= 2.2) {
    mInternals->program = clCreateProgramWithIL(mInternals->context, _makefile_opencl_program_Base_opencl_GPUReconstructionOCL2_cl_spirv, _makefile_opencl_program_Base_opencl_GPUReconstructionOCL2_cl_spirv_size, &ocl_error);
  } else
	{
		size_t program_sizes[1] = {_makefile_opencl_program_Base_opencl_GPUReconstructionOCL2_cl_src_size};
		char* programs_sources[1] = {_makefile_opencl_program_Base_opencl_GPUReconstructionOCL2_cl_src};
		mInternals->program = clCreateProgramWithSource(mInternals->context, (cl_uint) 1, (const char**) &programs_sources, program_sizes, &ocl_error);
	}
#endif // clang-format on

	return 0;
}
