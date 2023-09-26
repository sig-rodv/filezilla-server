#include "test_utils.hpp"

#include "../src/filezilla/util/parser.hpp"
#include "../src/filezilla/hostaddress.hpp"

/*
 * This testsuite asserts the correctness of the parser class of functions.
 */

class parser_test final : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(parser_test);
	CPPUNIT_TEST(test_ipv6);
	CPPUNIT_TEST_SUITE_END();

public:
	void test_ipv6();
};

CPPUNIT_TEST_SUITE_REGISTRATION(parser_test);


void parser_test::test_ipv6()
{
	using namespace fz;

	hostaddress::ipv6_host ipv6{9, 8, 7, 6, 5, 4, 3, 2};

	{
		std::string_view ip = "::1";
		util::parseable_range r(ip);

		CPPUNIT_ASSERT(parse_ip(r, ipv6) && eol(r));

		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0), ipv6[0]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0), ipv6[1]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0), ipv6[2]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0), ipv6[3]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0), ipv6[4]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0), ipv6[5]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0), ipv6[6]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(1), ipv6[7]);
	}

	{
		std::string_view ip = "123:456:789::8765:4321";
		util::parseable_range r(ip);

		CPPUNIT_ASSERT(parse_ip(r, ipv6) && eol(r));

		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x0123), ipv6[0]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x0456), ipv6[1]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x0789), ipv6[2]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x0000), ipv6[3]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x0000), ipv6[4]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x0000), ipv6[5]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x8765), ipv6[6]);
		CPPUNIT_ASSERT_EQUAL(std::uint16_t(0x4321), ipv6[7]);
	}

}
