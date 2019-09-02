#ifndef LIBYURI__PARSER_H_
#define LIBYURI__PARSER_H_
#include <iostream>
#include <string>

bool parse_space(const std::string &str, size_t &off) {
  while (isspace(str[off]) && off < str.size())off++;
  return off < str.size();
}

#define ERROR std::cerr << __FILE__ << ':' << __LINE__ << '[' << __PRETTY_FUNCTION__ << ']'
#define PARSE_ERROR(what) ERROR << "parse " << what << " failed. str = " << str << ", off = " << off <<'(' << str[off-2] << str[off-1] << str[off] << str[off+1] << str[off + 2]<< ')' << "\n"
#define PARSE_SPACE() if (!parse_space(str, off)){PARSE_ERROR("space");return false;}

bool parse_curly_left(const std::string &str, size_t &off) {
  PARSE_SPACE();
  if (str[off] != '{')return false;
  off++;
  return true;
}

bool parse_curly_right(const std::string &str, size_t &off) {
  PARSE_SPACE();
  if (str[off] != '}')return false;
  off++;
  return true;
}

bool parse_bracket_left(const std::string &str, size_t &off) {
  PARSE_SPACE();
  if (str[off] != '[')return false;
  off++;
  return true;
}

bool parse_bracket_right(const std::string &str, size_t &off) {
  PARSE_SPACE();
  if (str[off] != ']')return false;
  off++;
  return true;
}

bool parse_colon(const std::string &str, size_t &off) {
  PARSE_SPACE();
  if (str[off] != ':')return false;
  off++;
  return true;
}

bool parse_comma(const std::string &str, size_t &off) {
  PARSE_SPACE();
  if (str[off] != ',')return false;
  off++;
  return true;
}

#endif
