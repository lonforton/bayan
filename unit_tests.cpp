#define BOOST_TEST_MODULE bayan_test_module

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>

#include "bayan.h"

using namespace boost::filesystem;

struct BayanFixtureTest
{
  BayanFixtureTest() 
  {  

  std::system("mkdir test_root\\test_A\\test_A_1 2>NUL");
  std::system("mkdir test_root\\test_A\\test_A_2 2>NUL");
  std::system("mkdir test_root\\test_A\\test_A_3 2>NUL");
  std::system("mkdir test_root\\test_B\\test_B_1 2>NUL");
  std::system("mkdir test_root\\test_B\\test_B_2 2>NUL");

  ofstream ofs{"test_root\\test_A\\test_A_1\\C_1"};
  ofs << "C_1 C_1 C_1 C_1 C_1";
  ofs.close();

  ofs.open("test_root\\test_A\\test_A_1\\C_2");
  ofs << "C_2 C_2 C_2 C_2 C_2";
  ofs.close();

  ofs.open("test_root\\test_A\\test_A_2\\C_3");
  ofs << "C_3 C_3 C_3 C_3 C_3";
  ofs.close();

  ofs.open("test_root\\test_A\\test_A_2\\C_4");
  ofs << "C_4 C_4 C_4 C_4 C_4";
  ofs.close();

  ofs.open("test_root\\test_B\\test_B_1\\C_1");
  ofs << "C_1 C_1 C_1 C_1 C_1";
  ofs.close();

  ofs.open("test_root\\test_B\\test_B_1\\C_2");
  ofs << "C_2 C_2 C_2 C_2 C_2";
  ofs.close();

  ofs.open("test_root\\test_B\\test_B_2\\C_4");
  ofs << "C_4 C_4 C_4 C_4 C_4";
  ofs.close();

  ofs.open("test_root\\test_B\\test_B_2\\C_5");
  ofs << "C_5 C_5 C_5 C_5 C_5";
  ofs.close();
  }  

};

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

BOOST_AUTO_TEST_SUITE(bayan_test_suite)

BOOST_FIXTURE_TEST_CASE(bayan_input_test, BayanFixtureTest)
{
  std::vector<std::string> str{};
  Bayan bayan("test_root", str, true, 1, "'*'", 5, "crc32");  
  
  boost::test_tools::output_test_stream output;
  {
    cout_redirect redirect(output.rdbuf());
    auto duplicate_files = bayan.get_duplicate_files();
    for (auto itr : duplicate_files)
    {
      std::cout << std::endl;
      for (const auto &se : itr)
      {
        std::cout << se << std::endl;
      }
    }
  }

  BOOST_CHECK(output.is_equal("\n\
test_root\\test_A\\test_A_1\\C_1\n\
test_root\\test_B\\test_B_1\\C_1\n\
\n\
test_root\\test_A\\test_A_1\\C_2\n\
test_root\\test_B\\test_B_1\\C_2\n\
\n\
test_root\\test_A\\test_A_2\\C_4\n\
test_root\\test_B\\test_B_2\\C_4\n"));
}

BOOST_FIXTURE_TEST_CASE(bayan_exclude_test_1, BayanFixtureTest)
{
  std::vector<std::string> excludes;
  excludes.push_back("test_A");
  Bayan bayan("test_root", excludes, true, 1, "'*'", 5, "crc32");  
  
  boost::test_tools::output_test_stream output;
  {
    cout_redirect redirect(output.rdbuf());
    auto duplicate_files = bayan.get_duplicate_files();
    for (auto itr : duplicate_files)
    {
      std::cout << std::endl;
      for (const auto &se : itr)
      {
        std::cout << se << std::endl;
      }
    }
  }

  BOOST_CHECK(output.is_equal(""));
}

BOOST_FIXTURE_TEST_CASE(bayan_exclude_test_2, BayanFixtureTest)
{
  std::vector<std::string> excludes;
  excludes.push_back("test_A_1");
  Bayan bayan("test_root", excludes, true, 1, "'*'", 5, "crc32");  
  
  boost::test_tools::output_test_stream output;
  {
    cout_redirect redirect(output.rdbuf());
    auto duplicate_files = bayan.get_duplicate_files();
    for (auto itr : duplicate_files)
    {
      std::cout << std::endl;
      for (const auto &se : itr)
      {
        std::cout << se << std::endl;
      }
    }
  }

  BOOST_CHECK(output.is_equal("\n\
test_root\\test_A\\test_A_2\\C_4\n\
test_root\\test_B\\test_B_2\\C_4\n"));
}

BOOST_AUTO_TEST_SUITE_END()
