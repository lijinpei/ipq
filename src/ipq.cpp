#include "interval_tree.hpp"
#include "location.hpp"

#ifdef BTREE
#include "btree_map.hpp"
using IntervalTree = ipq::IntervalTree<uint32_t, ipq::Location,
                  ipq::BTreeMap<uint32_t, std::pair<uint32_t, ipq::Location>>>;
#else
using IntervalTree = ipq::IntervalTree<uint32_t, ipq::Location,
                  std::map<uint32_t, std::pair<uint32_t, ipq::Location>>>;
#endif

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

struct CsvLine {
  uint32_t start_ip, end_ip;
  std::string country_code, country, province, city;
};

struct CountryInfo {
  std::string code;
  std::string name;
  std::map<std::pair<std::string, std::string>, int> cities;
  std::vector<std::pair<std::string, std::string>> city_names;
  CountryInfo(const std::string& code, const std::string& name)
      : code(code), name(name), city_names(1) {}
  CountryInfo() : CountryInfo("", "") {}
};

int country_number;
std::map<std::string, int> country_code;
std::vector<std::string> count_name(1);
std::vector<CountryInfo> country_infos(1);

const int BUF_SIZE = 100;
char code_buf[BUF_SIZE], country_buf[BUF_SIZE], province_buf[BUF_SIZE],
    city_buf[BUF_SIZE];
// ipq::SegmentTree<ipq::Location> geo_ip(std::numeric_limits<uint32_t>::min(),
// std::numeric_limits<uint32_t>::max());
ipq::IntervalTree<uint32_t, ipq::Location,
                  std::map<uint32_t, std::pair<uint32_t, ipq::Location>>>
    geo_ip;

uint32_t parse_ip(const std::string& ip) {
  size_t p = 0;
  uint32_t ret = 0;
  for (int i = 0; i < 4; ++i) {
    size_t np = ip.find('.', p);
    uint32_t v = 0;
    for (; p < np; ++p) {
      v = v * 10 + (ip[p] - '0');
    }
    ret = (ret << 8) | v;
    p = np;
  }
  return ret;
}

int main(int, const char** argv) {
  std::ifstream csv_file((argv[1]));
  if (!csv_file) {
    std::cout << "wrong input csv file: " << argv[1] << std::endl;
    return 1;
  }
  auto get_country_code = [&](const std::string& code,
                              const std::string& country) -> int {
    auto& ret = country_code[code];
    if (!ret) {
      ret = country_code.size();
      count_name.emplace_back(code);
      country_infos.emplace_back(code, country);
    }
    return ret;
  };
  auto get_city_code = [&](int code, const std::string& province,
                           const std::string& city) {
    auto& info = country_infos[code];
    auto key = std::make_pair(province, city);
    auto& ret = info.cities[key];
    if (!ret) {
      ret = info.cities.size();
      info.city_names.emplace_back(province, city);
    }
    return ret;
  };
  int lines_read = 0;
  while (true) {
    uint32_t start_ip, end_ip;
    std::string code, country, province, city;
    csv_file.get();
    if (csv_file.eof()) {
      break;
    }
    ++lines_read;
    csv_file >> start_ip;
    csv_file.get();
    csv_file.get();
    csv_file.get();
    csv_file >> end_ip;
    csv_file.get();
    csv_file.get();
    csv_file.get();
    getline(csv_file, code, '"');
    csv_file.get();
    csv_file.get();
    getline(csv_file, country, '"');
    csv_file.get();
    csv_file.get();
    getline(csv_file, province, '"');
    csv_file.get();
    csv_file.get();
    getline(csv_file, city, '"');
    csv_file.get();
    csv_file.get();
    int country_code = get_country_code(code, country);
    int city_code = get_city_code(country_code, province, city);
    ipq::Location loc(country_code, city_code);
    geo_ip.update(start_ip, end_ip, loc);
  }
  std::cout << "ip location informations read: " << lines_read << std::endl;
  auto get_ip = [&]() -> uint32_t {
    std::string ip;
    std::cin >> ip;
    if (ip.find('.') != ip.npos) {
      return parse_ip(ip);
    } else {
      return std::stoll(ip);
    }
  };
  while (true) {
    std::string command;
    std::cin >> command;
    if (command == "query") {
      uint32_t ip = get_ip();
      ipq::Location* loc = geo_ip.find(ip);
      if (!loc) {
        std::cout << "not found" << std::endl;
      } else {
        int country_code = loc->getProvinceCode();
        int city_code = loc->getCountryCode();
        std::cout << "country code: " << country_infos[country_code].code
                  << " country name: " << country_infos[country_code].name
                  << std::endl;
        auto& pc = country_infos[country_code].city_names[city_code];
        std::cout << "province: " << pc.first << " city : " << pc.second
                  << std::endl;
      }
    } else if (command == "update") {
      uint32_t ip1 = get_ip();
      uint32_t ip2 = get_ip();
      std::string code, country, province, city;
      std::cin >> code >> country >> province >> city;
      int country_code = get_country_code(code, country);
      int city_code = get_city_code(country_code, province, city);
      geo_ip.update(ip1, ip2, ipq::Location(country_code, city_code));
    } else if (command == "delete") {
      uint32_t ip1 = get_ip();
      uint32_t ip2 = get_ip();
      geo_ip.remove(ip1, ip2);
    } else {
      std::cout << "unknown command" << std::endl;
    }
  }
}
