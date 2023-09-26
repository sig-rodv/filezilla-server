#include <libfilezilla/util.hpp>
#include <libfilezilla/encode.hpp>
#include <libfilezilla/local_filesys.hpp>
#include <libfilezilla/recursive_remove.hpp>

#ifdef FZ_WINDOWS
#	include <fileapi.h>
#else
#	include <unistd.h>
#endif

#include "../src/filezilla/util/filesystem.hpp"

#include "test_utils.hpp"

/*
 * This testsuite asserts the correctness of the basic_path class.
 */

class basic_path_test final : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(basic_path_test);
	CPPUNIT_TEST(test_normalize<std::string>);
	CPPUNIT_TEST(test_normalize<std::wstring>);
	CPPUNIT_TEST_SUITE_END();

public:
	template <typename String>
	void test_normalize();
};

CPPUNIT_TEST_SUITE_REGISTRATION(basic_path_test);

#define S(string) fzS(typename String::value_type, string)

using namespace fz::util::fs;

template <typename String>
void basic_path_test::test_normalize()
{
	using upath = basic_path<String, path_format::unix_format>;
	using wpath = basic_path<String, path_format::windows_format>;

	upath u_not_normal = S("/..//./1/2/../4///");
	upath u_expected = S("/1/4");

	CPPUNIT_ASSERT_EQUAL(u_not_normal.normalized().str(), u_expected.str());

	wpath w1_not_normal = S("//server/share/\\\\/1/./../2\\3/..\\4/5\\\\");
	wpath w1_expected = S("\\\\server\\share\\2\\4\\5");

	CPPUNIT_ASSERT_EQUAL(w1_not_normal.normalized().str(), w1_expected.str());

	wpath w2_not_normal = S("C://\\\\/1/../2\\3/..\\4/./5\\\\");
	wpath w2_expected = S("C:\\2\\4\\5");

	CPPUNIT_ASSERT_EQUAL(w2_not_normal.normalized().str(), w2_expected.str());
}
