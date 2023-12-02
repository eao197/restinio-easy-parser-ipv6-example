#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN 
#include <doctest/doctest.h>

#include <restinio/helpers/easy_parser.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace example
{

class hex_uint16_collector_t
{
	std::uint16_t m_value{};

public:
	void
	push_back( char ch )
	{
		const auto v = [ch]() -> std::uint16_t {
				switch( ch )
				{
				case '0': return 0u;
				case '1': return 1u;
				case '2': return 2u;
				case '3': return 3u;
				case '4': return 4u;
				case '5': return 5u;
				case '6': return 6u;
				case '7': return 7u;
				case '8': return 8u;
				case '9': return 9u;
				case 'a': case 'A': return 10u;
				case 'b': case 'B': return 11u;
				case 'c': case 'C': return 12u;
				case 'd': case 'D': return 13u;
				case 'e': case 'E': return 14u;
				case 'f': case 'F': return 15u;
				}
				throw std::runtime_error{ "illegal hex digit" };
			}();
		m_value <<= 4u;
		m_value |= v;
	}

	[[nodiscard]] std::uint16_t
	value() const noexcept { return m_value; }
};

struct set_pos_from_end_t
{
	std::size_t m_value;
};

[[nodiscard]]
set_pos_from_end_t
operator""_from_end(unsigned long long v) { return { v }; }

using ipv6_bytes_t = std::array< std::uint16_t, 8u >;

struct ipv4_by_bytes_t
{
	std::uint8_t m_b0{};
	std::uint8_t m_b1{};
	std::uint8_t m_b2{};
	std::uint8_t m_b3{};
};

class ipv6_bytes_collector_t
{
	ipv6_bytes_t m_value;
	std::size_t m_pos{};

public:
	ipv6_bytes_collector_t() = default;
	ipv6_bytes_collector_t(
		std::uint16_t v0, std::uint16_t v1, std::uint16_t v2, std::uint16_t v3,
		std::uint16_t v4, std::uint16_t v5, std::uint16_t v6, std::uint16_t v7 )
		: m_value{ v0, v1, v2, v3, v4, v5, v6, v7 }
		, m_pos{ m_value.size() }
	{}

	void
	push_back( std::uint16_t h16v )
	{
		if( m_pos >= m_value.size() )
			throw std::runtime_error{
				"ipv6_bytes_collector_t::push_back(std::uint16_t): m_pos is already "
				"at the end of m_value"
			};

		m_value[ m_pos ] = h16v;
		++m_pos;
	}

	void
	push_back( set_pos_from_end_t what )
	{
		if( what.m_value > m_value.size() )
			throw std::runtime_error{
				"ipv6_bytes_collector_t::push_back(set_pos_from_end_t): value is too big: "
				+ std::to_string( m_value.size() )
			};

		m_pos = m_value.size() - what.m_value;
	}

	void
	push_back( ipv4_by_bytes_t what )
	{
		if( m_pos + 2u > m_value.size() )
			throw std::runtime_error{
				"ipv6_bytes_collector_t::push_back(ipv4_by_bytes_t): m_pos is too big: "
				+ std::to_string( m_pos )
			};

		m_value[ m_pos ] = (std::uint16_t{ what.m_b0 } << 8u) | std::uint16_t{ what.m_b1 };
		++m_pos;

		m_value[ m_pos ] = (std::uint16_t{ what.m_b2 } << 8u) | std::uint16_t{ what.m_b3 };
		++m_pos;
	}

	[[nodiscard]]
	const ipv6_bytes_t &
	value() const noexcept{ return m_value; }

	[[nodiscard]]
	friend bool
	operator==( const ipv6_bytes_collector_t & a, const ipv6_bytes_collector_t & b ) noexcept
	{
		return a.value() == b.value();
	}
};

std::ostream &
operator<<( std::ostream & to, const example::ipv6_bytes_collector_t & collector )
{
	const auto & what = collector.value();

	to << "[";
	to << fmt::format( "0x{:x}", fmt::join(what, ", 0x") ) << "]";
	return to;
}

} /* namespace example */

namespace restinio::easy_parser
{

template<>
struct result_value_wrapper< ::example::hex_uint16_collector_t >
{
	using result_type = ::example::hex_uint16_collector_t;
	using value_type = char;
	using wrapped_type = result_type;

	static void
	as_result( wrapped_type & to, result_type && what )
	{
		to = what;
	}

	static void
	to_container( wrapped_type & to, value_type && what )
	{
		to.push_back( std::move(what) );
	}

	[[nodiscard]]
	static result_type &&
	unwrap_value( wrapped_type & v )
	{
		return std::move(v);
	}
};

template<>
struct result_value_wrapper< ::example::ipv6_bytes_collector_t >
{
	using result_type = ::example::ipv6_bytes_collector_t;
	using value_type = std::uint16_t;
	using wrapped_type = result_type;

	static void
	as_result( wrapped_type & to, result_type && what )
	{
		to = what;
	}

	template< typename T >
	static void
	to_container( wrapped_type & to, T && what )
	{
		to.push_back( std::forward<T>(what) );
	}

	[[nodiscard]]
	static result_type &&
	unwrap_value( wrapped_type & v )
	{
		return std::move(v);
	}
};

} /* namespace restinio::easy_parser */

namespace example
{

namespace ep = restinio::easy_parser;

[[nodiscard]]
auto
h16_p()
{
	return ep::produce< std::uint16_t >(
			ep::produce< hex_uint16_collector_t >(
				ep::repeat( 1u, 4u, ep::hexdigit_p() >> ep::to_container() ) )
				>> ep::convert( []( const hex_uint16_collector_t & c ) { return c.value(); } )
				>> ep::as_result()
		);
}

[[nodiscard]]
restinio::expected_t< std::uint16_t, ep::parse_error_t >
try_parse_h16( std::string_view what )
{
	return ep::try_parse( what, h16_p() );
}

[[nodiscard]]
auto
double_colon( set_pos_from_end_t new_pos )
{
	return ep::exact_p( "::" ) >> ep::just( new_pos ) >> ep::to_container();
}

[[nodiscard]]
auto
ipv4_p()
{
	const auto u8_p = ep::non_negative_decimal_number_p< std::uint8_t >();
	return ep::produce< ipv4_by_bytes_t >(
			u8_p >> &ipv4_by_bytes_t::m_b0,
			ep::symbol( '.' ),
			u8_p >> &ipv4_by_bytes_t::m_b1,
			ep::symbol( '.' ),
			u8_p >> &ipv4_by_bytes_t::m_b2,
			ep::symbol( '.' ),
			u8_p >> &ipv4_by_bytes_t::m_b3
		);
}

/*
 * Uses the following grammar (see https://tools.ietf.org/html/rfc3986#appendix-A):
@verbatim
   IPv6address   =                            6( h16 ":" ) ls32
                 /                       "::" 5( h16 ":" ) ls32
                 / [               h16 ] "::" 4( h16 ":" ) ls32
                 / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
                 / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
                 / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
                 / [ *4( h16 ":" ) h16 ] "::"              ls32
                 / [ *5( h16 ":" ) h16 ] "::"              h16
                 / [ *6( h16 ":" ) h16 ] "::"

   h16           = 1*4HEXDIG
   ls32          = ( h16 ":" h16 ) / IPv4address
@endverbatim
 */
[[nodiscard]]
auto
ipv6_p()
{
	const auto h16_with_colon = ep::sequence(
			h16_p() >> ep::to_container(),
			ep::symbol(':'),
			ep::not_clause( ep::symbol(':') )
		);

	const auto ls32 = ep::alternatives(
			ep::sequence(
				h16_p() >> ep::to_container(),
				ep::symbol(':'),
				h16_p() >> ep::to_container()
			),
			ipv4_p() >> ep::to_container()
		);

	return ep::produce< ipv6_bytes_collector_t >(
				ep::alternatives(
					ep::sequence(
						ep::repeat( 6u, 6u, h16_with_colon ),
						ls32
					),
					ep::sequence(
						double_colon( 7_from_end ),
						ep::repeat( 5u, 5u, h16_with_colon ),
						ls32
					),
					ep::sequence(
						ep::maybe(
							h16_p() >> ep::to_container()
						),
						double_colon( 6_from_end ),
						ep::repeat( 4u, 4u, h16_with_colon ),
						ls32
					),
					ep::sequence(
						ep::maybe(
							ep::repeat( 0u, 1u, h16_with_colon ),
							h16_p() >> ep::to_container()
						),
						double_colon( 5_from_end ),
						ep::repeat( 3u, 3u, h16_with_colon ),
						ls32
					),
					ep::sequence(
						ep::maybe(
							ep::repeat( 0u, 2u, h16_with_colon ),
							h16_p() >> ep::to_container()
						),
						double_colon( 4_from_end ),
						ep::repeat( 2u, 2u, h16_with_colon ),
						ls32
					),
					ep::sequence(
						ep::maybe(
							ep::repeat( 0u, 3u, h16_with_colon ),
							h16_p() >> ep::to_container()
						),
						double_colon( 3_from_end ),
						h16_with_colon,
						ls32
					),
					ep::sequence(
						ep::maybe(
							ep::repeat( 0u, 4u, h16_with_colon ),
							h16_p() >> ep::to_container()
						),
						double_colon( 2_from_end ),
						ls32
					),
					ep::sequence(
						ep::maybe(
							ep::repeat( 0u, 5u, h16_with_colon ),
							h16_p() >> ep::to_container()
						),
						double_colon( 1_from_end ),
						h16_p() >> ep::to_container()
					),
					ep::sequence(
						ep::maybe(
							ep::repeat( 0u, 6u, h16_with_colon ),
							h16_p() >> ep::to_container()
						),
						double_colon( 0_from_end )
					)
				)
			);
}

[[nodiscard]]
restinio::expected_t< ipv6_bytes_collector_t, ep::parse_error_t >
try_parse( std::string_view what )
{
	return ep::try_parse( what, ipv6_p() );
}

} /* namespace example */

TEST_CASE( "h16" )
{
	{
		const auto r = example::try_parse_h16( "0" );
		REQUIRE( true == r.has_value() );

		REQUIRE( 0u == *r );
	}

	{
		const auto r = example::try_parse_h16( "0000" );
		REQUIRE( true == r.has_value() );

		REQUIRE( 0u == *r );
	}

	{
		const auto r = example::try_parse_h16( "1234" );
		REQUIRE( true == r.has_value() );

		REQUIRE( 0x1234u == *r );
	}

	{
		const auto r = example::try_parse_h16( "AbCe" );
		REQUIRE( true == r.has_value() );

		REQUIRE( 0xabceu == *r );
	}
}

TEST_CASE( "ipv6" )
{
	{
		const auto r = example::try_parse( "::" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "fe01::" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0xfe01u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "0:fe01::" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0xfe01u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "abcd:0:fe01::" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0xabcdu, 0x0u, 0xfe01u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "fe01::1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0xfe01u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "0:fe01::1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0xfe01u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "abcd:0:fe01::1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0xabcdu, 0x0u, 0xfe01u, 0x0u, 0x0u, 0x0u, 0x0u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::1:1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x1u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::255.255.255.255" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0xffffu, 0xffffu };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "0:ab::127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0xabu, 0x0u, 0x0u, 0x0u, 0x0u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::1001:1001:1001" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x1001u, 0x1001u, 0x1001u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::2002:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x2002u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "0:ab::3003:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0xabu, 0x0u, 0x0u, 0x0u, 0x3003u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::1001:1001:1001:1001" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x1001u, 0x1001u, 0x1001u, 0x1001u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::3003:2002:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x0u, 0x3003u, 0x2002u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "0:ab::4004:3003:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0xabu, 0x0u, 0x0u, 0x4004u, 0x3003u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::1001:1001:1001:1001:1001" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::4004:3003:2002:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x0u, 0x4004u, 0x3003u, 0x2002u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "0:ab::5005:4004:3003:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0xabu, 0x0u, 0x5005u, 0x4004u, 0x3003u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::1001:1001:1001:1001:1001:1001" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::5005:4004:3003:2002:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x0u, 0x5005u, 0x4004u, 0x3003u, 0x2002u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "ab::6006:5005:4004:3003:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0xabu, 0x0u, 0x6006u, 0x5005u, 0x4004u, 0x3003u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::1001:1001:1001:1001:1001:1001:1001" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "::6006:5005:4004:3003:2002:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x0u, 0x6006u, 0x5005u, 0x4004u, 0x3003u, 0x2002u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "1001:1001:1001:1001:1001:1001:1001:1001" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u, 0x1001u };
		REQUIRE( expected == *r );
	}

	{
		const auto r = example::try_parse( "7007:6006:5005:4004:3003:2002:127.0.0.1" );
		REQUIRE( true == r.has_value() );

		const example::ipv6_bytes_collector_t expected{ 0x7007u, 0x6006u, 0x5005u, 0x4004u, 0x3003u, 0x2002u, 0x7f00u, 0x1u };
		REQUIRE( expected == *r );
	}
}

