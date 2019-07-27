#include <set>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>

using namespace boost::filesystem;
using boost::uuids::detail::md5;

class Bayan
{
public:
  enum class HashAlgorithm { crc_32, md_5 };

  Bayan(std::string dirs, std::vector<std::string> exclude, int level, int size, std::string mask, int block_size, std::string algorithm) 
  : _dirs(dirs), _exclude_dirs(exclude), _level(level),  _size(size), _mask(mask), _block_size(block_size)
  {
    if (algorithm.find("md5") != std::string::npos)
    {
      _algorithm = HashAlgorithm::md_5;
    }
    else
    {
      _algorithm = HashAlgorithm::crc_32;
    }
    
    boost::replace_all(_mask, "'.'", "[.]");
    boost::replace_all(_mask, "'*'", ".*");    
    boost::replace_all(_mask, "'?'", ".");
  }

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
      return std::to_string(result.checksum());
    }
  }

  bool is_path_in_results(const std::vector<std::set<std::string>>& vec, const std::string& path) const
  {
  for (auto const &entry : vec)
  {
    if (std::find_if(entry.begin(), entry.end(), 
    [path](std::string value) { return value == path; }) != entry.end())
    {
      return true;
    }
  }
    return false;
  }

std::string read_from_cache(uint32_t from_position, const directory_iterator& dir_it)
{
  std::string cache_string;
  std::string path_string = (*dir_it).path().string();
  auto item_iter = _files_cache.find(path_string);
  if(item_iter != _files_cache.end())
  {
    cache_string = (*item_iter).second.substr(from_position, _block_size);
  }

  return cache_string;
}

std::pair<std::string, std::string> read_from_position(uint32_t from_position, const directory_iterator &first_dir_it, const directory_iterator &second_dir_it)
{
  std::string first_result(_block_size, '\0');
  std::string second_result(_block_size, '\0');

  ifstream ifs_1{*first_dir_it};
  ifstream ifs_2{*second_dir_it};

  std::string cache_string = read_from_cache(from_position, first_dir_it);

  if (cache_string.size() != _block_size)
  {
    ifs_1.seekg(from_position);
    if (ifs_1.peek() == EOF)
      return std::make_pair(std::string(""), std::string(""));

    ifs_1.read(&first_result[0], _block_size);
    add_to_files_cache(first_dir_it, first_result);
  }
  else
  {
    first_result = cache_string;
  }

  cache_string = read_from_cache(from_position, second_dir_it);

  if (cache_string.size() != _block_size)
  {
    ifs_2.seekg(from_position);
    if (ifs_1.peek() == EOF || ifs_2.peek() == EOF)
      return std::make_pair(std::string(""), std::string(""));

    ifs_2.read(&second_result[0], _block_size);
    add_to_files_cache(second_dir_it, second_result);
  }
  else 
  {
    second_result = cache_string;
  }

  return std::make_pair(first_result, second_result);
}

void add_to_files_cache(directory_iterator dir_it, std::string file_string)
{
  std::string path_string = (*dir_it).path().string();
  auto item_iter = _files_cache.find(path_string);
  if (item_iter != _files_cache.end())
  {
    (*item_iter).second += file_string;
  }
  else
  {
    _files_cache.insert(std::pair<std::string, std::string>(path_string, file_string));
  }
}

bool compare_by_blocks(const directory_iterator &first_dir_it, const directory_iterator &second_dir_it)
{
  int position = 0;
  std::pair<std::string, std::string> result_pair = read_from_position(position, first_dir_it, second_dir_it);

  while (get_block_hash(result_pair.first) == get_block_hash(result_pair.second))
  {
    position += _block_size;
    result_pair = read_from_position(position, first_dir_it, second_dir_it);
    if (result_pair.first.empty() || result_pair.second.empty())
      break;
  }

  return get_block_hash(result_pair.first) == get_block_hash(result_pair.second);
}

void skip_checked_files(directory_iterator first_dir_it, directory_iterator second_dir_it)
{
  while ((*second_dir_it).path() != (*first_dir_it).path())
  {
    ++second_dir_it;
  }
  ++second_dir_it;
}

bool check_conditions(const directory_iterator &dir_it) const
{
  const boost::regex my_filter(_mask);
  boost::smatch what;

  auto checked_path = (*dir_it).path();
  bool is_path_directory = is_directory(status(*dir_it));

  return is_path_in_results(_result_vec, checked_path.string())
  || (!is_path_directory && file_size(checked_path) < _size) 
  || boost::regex_match(checked_path.filename().string(), what, my_filter) 
  || (is_path_directory && std::find(_exclude_dirs.begin(), _exclude_dirs.end(), checked_path.filename().string()) != _exclude_dirs.end());     
}

std::vector<std::set<std::string>> get_duplicate_files()
{
  directory_iterator first_dir_it{_dirs};
  while (first_dir_it != directory_iterator{})
  {
    if (check_conditions(first_dir_it))
    {
      ++first_dir_it;
      continue;
    }

    if (is_directory(status(*first_dir_it)))
    {
      if (_level == 0)
      {
        std::string temp = _dirs;
        _dirs = (*first_dir_it).path().string();
        get_duplicate_files();
        _dirs = temp;
      }
      else
      {
        ++first_dir_it;
        continue;
      }
    }
    else
    {
      std::set<std::string> result_set;
      directory_iterator second_dir_it(_dirs);

      skip_checked_files(first_dir_it, second_dir_it);

      while (second_dir_it != directory_iterator{})
      {
        if (is_directory(status(*second_dir_it)) || (!is_directory(status(*second_dir_it)) && file_size((*second_dir_it).path()) < _size) )
        {
          ++second_dir_it;
          continue;
        }

        if (compare_by_blocks(first_dir_it, second_dir_it))
        {
          result_set.insert((*first_dir_it).path().string());
          result_set.insert((*second_dir_it).path().string());
        }
        ++second_dir_it;
      }
      if (!result_set.empty())
      {
        _result_vec.push_back(result_set);
      }
    }
    ++first_dir_it;
  }

  return _result_vec;
  }

private:
  std::string _dirs;
  std::vector<std::string> _exclude_dirs;
  int _level;
  uint64_t _size;
  std::string _mask;
  uint64_t _block_size;
  HashAlgorithm _algorithm;
  std::map<std::string, std::string> _files_cache;
  std::vector<std::set<std::string>> _result_vec;
};
