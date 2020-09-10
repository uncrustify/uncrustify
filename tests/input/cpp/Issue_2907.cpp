template< typename Enum > class Flags
{
  public:
    constexpr Flags() : value{ 0 } {}
    constexpr Flags( Enum f ) : value( static_cast< value_t >( f ) ) {}
    constexpr Flags( Flags const& ) = default;
}
