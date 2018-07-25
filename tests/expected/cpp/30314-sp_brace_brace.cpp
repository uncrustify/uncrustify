template<std::size_t _N, typename _Type, _Type... _Nums>
std::array<uint8_t, _N - 1> constexpr crypt_helper(uint8_t const inSeed, char const (&inString)[_N], std::integer_sequence<_Type, _Nums...>) {
	return { {crypt(_Nums, inSeed, static_cast<uint8_t>(inString[_Nums]))...} };
}
static std::array<double_t, Homology::kNumberOfStats> const m{ {
	0.3,
	0.6,
	1.0
} };
static std::array<double_t, Homology::kNumberOfStats> const m = {
	0.3,
	0.6,
	1.0
};
