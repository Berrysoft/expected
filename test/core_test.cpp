#define BOOST_TEST_MODULE CoreTest

#include <boost/test/unit_test.hpp>
#include <expected.hpp>

#define EXPECTED_SUCCEEDED(res, v) \
    BOOST_REQUIRE((res).value());  \
    BOOST_REQUIRE_EQUAL(*(res).value(), (v))

#define EXPECTED_SUCCEEDED_VOID(res) BOOST_REQUIRE((res).value())

#define EXPECTED_FAILED(res, v)   \
    BOOST_REQUIRE((res).error()); \
    BOOST_REQUIRE_EQUAL(*(res).error(), (v))

using namespace expected;

task<int, bool> return_task(int value)
{
    if (value > 0)
        co_return value;
    else
        co_return false;
}

BOOST_AUTO_TEST_CASE(return_test_succeeded)
{
    auto r = return_task(100).run();
    EXPECTED_SUCCEEDED(r, 100);
}

BOOST_AUTO_TEST_CASE(return_test_failed)
{
    auto r = return_task(-100).run();
    EXPECTED_FAILED(r, false);
}

task<int, int> return_task2(int value)
{
    if (value > 0)
        co_return value;
    else
        co_return eerror, -value;
}

BOOST_AUTO_TEST_CASE(return_test2_succeeded)
{
    auto r = return_task2(100).run();
    EXPECTED_SUCCEEDED(r, 100);
}

BOOST_AUTO_TEST_CASE(return_test2_failed)
{
    auto r = return_task2(-100).run();
    EXPECTED_FAILED(r, 100);
}
