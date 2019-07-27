#define BOOST_TEST_MODULE bayan_test_module

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <iostream>

#include "bayan.h"

BOOST_AUTO_TEST_SUITE(bayan_test_suite)

/**
 * @brief Helper redirection fot tests
 */
struct cout_redirect
{
  cout_redirect(std::streambuf *new_buffer)
      : old(std::cout.rdbuf(new_buffer))
  {
  }

  ~cout_redirect()
  {
    std::cout.rdbuf(old);
  }

private:
  std::streambuf *old;
};

BOOST_AUTO_TEST_CASE(bayan_input_test_1)
{
  BOOST_CHECK(1 == 1);
}

BOOST_AUTO_TEST_SUITE_END()
