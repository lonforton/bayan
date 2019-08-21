#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm//cxx11/one_of.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/bimap.hpp>

using namespace boost::filesystem;
using boost::uuids::detail::md5;

class Bayan
{
public:
  enum class HashAlgorithm
  {
    crc_32,
    md_5
  };

  Bayan(std::string dirs, std::vector<std::string> exclude, bool recursive, int size, std::string mask, int block_size, std::string algorithm)
      : _dirs(dirs), _exclude_dirs(exclude), _recursive(recursive), _size(size), _mask(mask), _block_size(block_size)
  {
    if (algorithm.find("md5") != std::string::npos)
    {
      _algorithm = HashAlgorithm::md_5;      
      _hash_block_size = 32;
    }
    else
    {
      _algorithm = HashAlgorithm::crc_32;      
      _hash_block_size = 8;
    }

    boost::replace_all(_mask, ".", "\\.");
    boost::replace_all(_mask, "'\\.'", ".");
    boost::replace_all(_mask, "'*'", ".*");
    boost::replace_all(_mask, "'?'", ".?");
  }

    std::vector<std::set<std::string>> get_duplicate_files()
  {
    std::vector<std::set<std::string>> result_vec;
    recursive_directory_iterator dir_it{_dirs};
    std::vector<path> paths_vector;
    const boost::regex my_filter(_mask);
    boost::smatch what;
    
    while (dir_it != recursive_directory_iterator{})
    {
        if( (is_directory(status(*dir_it))))
        {
          if (
          (!_recursive) 
          || boost::algorithm::one_of_equal(_exclude_dirs,  dir_it->path().filename()) 
          )
          {
            dir_it.no_push();            
          }
          ++dir_it;
          continue;
        }
        else if( !is_regular_file(status(*dir_it))
        || (file_size(dir_it->path()) < _size)
        ||  !boost::regex_match(dir_it->path().filename().string(), what, my_filter))
        {       
          ++dir_it;
          continue;        
        }

        paths_vector.push_back(dir_it->path());

      ++dir_it;
    }

    for (auto first_it = paths_vector.begin(); first_it != paths_vector.end(); ++first_it)
    {
      if (path_in_results(result_vec, *first_it))
      {
        continue;
      }

      std::set<std::string> result_set;
      for (auto second_it = first_it + 1; second_it != paths_vector.end(); ++second_it)
      {
        if (compare_by_blocks(*first_it, *second_it))
        {
          result_set.insert(first_it->string());
          result_set.insert(second_it->string());
        }
      }
      if (!result_set.empty())
      {
        result_vec.push_back(result_set);
      }
    }

    return result_vec;
  }

private:

  std::string get_md5_hash(const std::string &str) const
  {
    md5 hash;
    md5::digest_type digest;
    hash.process_bytes(str.data(), str.size());
    hash.get_digest(digest);
    const auto char_digest = reinterpret_cast<const char *>(&digest);
    std::string result;
    boost::algorithm::hex(char_digest, char_digest + sizeof(md5::digest_type), std::back_inserter(result));
    return result;
  }

  std::string get_block_hash(const std::string &str) const
  {
    if (_algorithm == HashAlgorithm::md_5)
    {
      return get_md5_hash(str);
    }
    else
    {
      boost::crc_32_type result;
      result.process_bytes(str.data(), str.length());
      std::stringstream stream;
      stream << std::hex << result.checksum();
      return stream.str();
    }
  }

  bool path_in_results(const std::vector<std::set<std::string>>& vec, const path &file_path)
  {
    for (auto const &entry : vec)
    {
      if (std::find_if(entry.begin(), entry.end(),
                       [&file_path](const std::string &value) { return value == file_path.string(); }) != entry.end())
      {
        return true;
      }
    }
    return false;
  }

  std::string read_from_cache(const path &block_path, const int block_number)
  {
    std::string result(_hash_block_size, '\0');
    auto item_iter = _files_cache.find(block_path.string());
    if (item_iter != _files_cache.end())
    {
      if(item_iter->second.size() > block_number * _hash_block_size)
      {
          return item_iter->second.substr(block_number * _hash_block_size, _hash_block_size);
      }
      else
      {
         return std::string();
      }      
    }
    else{
      return std::string();
    }

    return std::string();
  }

  void add_to_cache(const path &block_path, const std::string &result)
  {
    auto item_iter = _files_cache.find(block_path.string());
    if (item_iter != _files_cache.end())
    {
      (*item_iter).second += result;
    }
    else
    {
      _files_cache.insert(std::pair<std::string, std::string>(block_path.string(), result));
    }
  }

  std::string read_block(const uint32_t block_number, const path &block_path)
  {
    std::string cache_result(_block_size, '\0');
    std::string file_result(_block_size, '\0');

    cache_result = read_from_cache(block_path, block_number);
    if (!cache_result.empty())
    {
      return cache_result;
    }
    else
    {
      ifstream ifs{block_path};
      ifs.seekg(block_number * _block_size);
      if (ifs.peek() == EOF)
      {
        return std::string("");
      }
      else
      {
        ifs.read(&file_result[0], _block_size);
        std::string hash_result = get_block_hash(file_result); 
        add_to_cache(block_path, hash_result);
        return hash_result;
      }
    }

    return std::string("");
  }

  bool compare_by_blocks(const path &first_path, const path &second_path)
  {
    uint32_t block_number = 0;
    if(file_size(first_path) != file_size(second_path))
      return false;

    std::string block_1 = read_block(block_number, first_path);
    std::string block_2 = read_block(block_number, second_path);

    while (!block_1.empty() && !block_2.empty())
    {
      if (block_1 != block_2)
      {
        return false;
      }
      ++block_number;
      block_1 = read_block(block_number, first_path);
      block_2 = read_block(block_number, second_path);
    }

    return true;
  }

  std::string _dirs;
  std::vector<std::string> _exclude_dirs;
  bool _recursive;
  uint64_t _size;
  std::string _mask;
  uint64_t _block_size;
  HashAlgorithm _algorithm;
  std::map<std::string, std::string> _files_cache;
  uint64_t _hash_block_size;
};