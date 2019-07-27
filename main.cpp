#include <iostream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "bayan.h"

using namespace boost::program_options;
using namespace boost::filesystem;

void on_age(int age)
{
  std::cout << "On age: " << age << '\n';
}

int main(int argc, const char *argv[])
{
  variables_map vm;
  std::vector<std::string> exclude_files;
  try
  {
    options_description desc{"Options"};
    desc.add_options()
      ("help,h",     "Help screen")
      ("dirs,d",      value<std::string>()->default_value(std::string("./")), "Dirs")
      ("exclude,e",   value<std::vector<std::string> >(), "list of folders to exclude")
      ("level,l",     value<int>()->default_value(0), "Level")
      ("size,s",      value<int>()->default_value(1), "Size")
      ("mask,m",      value<std::string>()->default_value(""), "Mask")
      ("blocksize,b", value<int>()->default_value(5), "Block size")
      ("algorithm,a", value<std::string>()->default_value(std::string("crc32")), "Algorithm");
    
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) 
    {
      std::cout << desc << '\n';
      return 0;
    }
    if (vm.count("dirs"))
      std::cout << "Dir: " << vm["dirs"].as<std::string>() << '\n';
    if (vm.count("exclude"))
    {
        exclude_files= vm["exclude"].as<std::vector<std::string> >();
        std::cout << "Exclude dirs: ";
        for(auto file : exclude_files)
        {
          std::cout << file << " ";
        }
        std::cout << std::endl;
    }
    if (vm.count("level"))
      std::cout << "Level: " << vm["level"].as<int>() << '\n';
    if (vm.count("size"))
      std::cout << "Size: " << vm["size"].as<int>() << '\n';
    if (vm.count("mask"))
      std::cout << "Mask: " << vm["mask"].as<std::string>() << '\n';
    if (vm.count("blocksize"))
      std::cout << "Blocksize: " << vm["blocksize"].as<int>() << '\n';
    if (vm.count("algorithm"))
      std::cout << "Algorithm: " << vm["algorithm"].as<std::string>() << '\n';

  }
  catch (const error &ex)
  {
    std::cerr << ex.what() << '\n';
    return 0;
  }

  Bayan bayan( vm["dirs"].as<std::string>(), exclude_files, vm["level"].as<int>(), 
         vm["size"].as<int>(), vm["mask"].as<std::string>(), vm["blocksize"].as<int>(), vm["algorithm"].as<std::string>());

  auto duplicate_files = bayan.get_duplicate_files();
  for (auto itr : duplicate_files)
  {
    std::cout << std::endl;
    for(const auto& se : itr)
    {
      std::cout << se << std::endl;
    }    
  }
}