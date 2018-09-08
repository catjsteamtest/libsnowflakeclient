/*
 * Copyright (c) 2017-2018 Snowflake Computing, Inc. All rights reserved.
 */

#include "Util.hpp"
#include "JwtException.hpp"
#include "../util/Base64.hpp"
#include <vector>

namespace Snowflake
{
namespace Jwt
{
std::string CJSONOperation::serialize(cJSON *root)
{
  char *json_str = snowflake_cJSON_Print(root);
  if (!json_str)
  {
    throw JwtMemoryAllocationFailure();
  }
  size_t json_str_len = strlen(json_str);
  size_t buf_len = Client::Util::Base64::encodedLength(json_str_len);
  std::vector<char> buffer(buf_len, 0);
  size_t len = Client::Util::Base64::encodeUrl(json_str, json_str_len, buffer.data());

  snowflake_cJSON_free(json_str);

  // remove the end of padding
  // TODO This is an ugly hack, change the Base64::encodeUrl() later
  while (buffer[len - 1] == '=') len--;

  return std::string(buffer.data(), len);
}

cJSON *CJSONOperation::parse(const std::string &text)
{
  // Add padding
  // TODO This is an ugly hack, change the Base64::decodeUrl() later
  std::string in_text = text;
  size_t pad_len = in_text.length() % 4;
  in_text.append(pad_len, '=');

  // Base 64 decode text
  size_t decode_len = Client::Util::Base64::decodedLength(in_text.length());
  std::vector<char> decoded(decode_len, 0);
  decode_len = Client::Util::Base64::decodeUrl(in_text.c_str(), in_text.length(), decoded.data());
  if ((ssize_t) decode_len == -1) throw JwtParseFailure();

  // Parse the decode string to object
  cJSON *root = snowflake_cJSON_Parse(decoded.data());
  if (root == nullptr) throw JwtParseFailure();

  return root;
}

void CJSONOperation::addOrReplaceJSON(cJSON *root, std::string key, cJSON *item)
{
  if (item == nullptr)
  {
    throw JwtMemoryAllocationFailure();
  }

  bool contains = snowflake_cJSON_HasObjectItem(root, key.c_str());

  if (!contains)
  {
    snowflake_cJSON_AddItemToObject(root, key.c_str(), item);
  }

  snowflake_cJSON_ReplaceItemInObject(root, key.c_str(), item);
}


}
}