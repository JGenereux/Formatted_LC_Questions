#include <curl/curl.h>
#include <string.h>

#include <unordered_set>
#include <iostream>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

typedef struct Response
{
  char *string;
  size_t size;
};

size_t write_chunk(void *data, size_t size, size_t nmemb, void *userData);

void formatResponse(char *response);
std::string FormatHTMLToString(const std::string &response);

void CreateJSON(json *response);

int main()
{
  std::string questionName = "";
  std::cout << "Enter Leetcode question name: " << std::endl;
  std::cin >> questionName;

  CURL *curl;
  CURLcode result;

  // Initialize CURL
  curl = curl_easy_init();
  if (curl == nullptr)
  {
    std::cerr << "HTTP REQUEST FAILED: curl_easy_init() failed!" << std::endl;
    return -1;
  }
  else
  {
    std::cout << "Curl initialized successfully!" << std::endl;
  }

  Response response;
  response.string = (char *)malloc(1);
  response.size = 0;

  // Set options for the HTTP request
  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://leetcode.com/graphql");

  // Set Post data (like JSON body) to match leetcode graph ql query
  json query = {
      {"query", "query questionData($titleSlug: String!) { question(titleSlug: $titleSlug) { title content difficulty topicTags { name } hints } }"},
      {"variables", {
                        {"titleSlug", questionName} // This can now be easily modified
                    }}};

  const std::string postData = query.dump();
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

  // Set headers for JSON data
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  std::string referer = "Referrer: https://leetcode.com/problems/" + questionName + "/";
  headers = curl_slist_append(headers, referer.c_str());

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  /**
   * WriteFunction allows for specifying a callback function
   * Curl_easy_perfrom will call this function repeatedly
   * Each time it is called the pointer is passed to a new chunk of response
   * string
   */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);

  // Address of response string is passed in write_chunk as userData
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  // Perform the HTTP request
  result = curl_easy_perform(curl);
  if (result != CURLE_OK)
  {
    std::cerr << "Error: " << curl_easy_strerror(result) << std::endl;
    curl_easy_cleanup(curl);
    return -1;
  }

  formatResponse(response.string);
  free(response.string);
  // Cleanup
  curl_easy_cleanup(curl);
  return 0;
}

// returns number of bytes in the chunk
//  data is set to a ptr that points to block of data recieved in this chunk
//  nmemb is the number of bytes in the block of data
// userData points to what we want (points to where the response string is stored)
size_t write_chunk(void *data, size_t size, size_t nmemb, void *userData)
{
  // size is always 1
  size_t real_size = size * nmemb;

  Response *response = (Response *)userData;
  // allocate more space for chunk that was recieved
  // response->size is size of existing mem and real_size is the size recieved and +1 accounts for null
  char *ptr = (char *)realloc(response->string, response->size + real_size + 1);

  if (ptr == nullptr)
  {
    std::cerr << "Problem reallocating space for chunk recieved" << std::endl;
    return 0;
  }
  // set response string to the new (larger) memory address
  response->string = ptr;
  // append new porition onto existing string
  memcpy(&(response->string[response->size]), data, real_size);
  // update strings size
  response->size += real_size;
  // append null character
  response->string[response->size] = '\0';
  return real_size;
}

/**
 * Returns a map containing the following tags stored as keys
 * and their description as their value.
 *
 * title content difficulty topicTags { name } hints
 *
 * Assumes json response will use the tags in the given order above.
 */
void formatResponse(char *response)
{
  std::vector<std::string> currentTags = {"title", "content", "difficulty", "topicTags", "hints"};

  try
  {
    json parsed = json::parse(response);
    json question = parsed["data"]["question"];

    for (const auto &tag : currentTags)
    {
      if (question.contains(tag) && tag == "topicTags")
      {
        std::vector<std::string> topics;
        for (auto topic : question[tag])
        {
          topics.push_back(topic["name"]);
        }
        question[tag] = topics;
        continue;
      }
      if (question.contains(tag) && tag == "hints")
      {
        if (question[tag][0].size() == 0)
        {
          continue;
        }
        question[tag][0] = FormatHTMLToString(question[tag][0]);
        continue;
      }
      if (question.contains(tag))
      {
        question[tag] = FormatHTMLToString(question[tag]);
      }
    }
    CreateJSON(&question);
  }
  catch (json::parse_error &e)
  {
    std::cerr << "Parse error: " << e.what() << std::endl;
    return;
  }
}

std::string FormatHTMLToString(const std::string &response)
{
  int i = 0;
  std::string result = "";

  while (i < response.length())
  {
    // check for HTML elements
    if (response[i] == '<')
    {
      while (response[i] != '>')
      {
        i++;
      }
      i++;
      continue;
    }

    // check for &lt; (<) , &gt (>);
    if (i < response.length() - 4 && (response.substr(i, 4) == "&lt;" || response.substr(i, 4) == "&gt;"))
    {
      std::string expression = response.substr(i, 4);
      if (expression == "&lt;")
      {
        result += "<";
      }
      else if (expression == "&gt;")
      {
        result += ">";
      }
      i += 4;
      continue;
    }

    // check for &amp (&)
    if (i < response.length() - 5 && (response.substr(i, 5) == "&amp;"))
    {
      result += "&";
      i += 5;
      continue;
    }

    // check for &#39;s
    if (i < response.length() - 6 && response.substr(i, 6) == "&#39;s")
    {
      i += 6;
      continue;
    }

    // check for &nbsp; tags
    if (i < response.length() - 6 && response.substr(i, 6) == "&nbsp;")
    {
      i += 6;
      continue;
    }

    result += (response[i]);
    i++;
  }
  return result;
}

void CreateJSON(json *response)
{
  // filter out invalid characters from title
  std::string title = (*response)["title"];
  const std::string invalid_chars = "\\/:*?\"<>|";
  for (char c : invalid_chars)
  {
    std::replace(title.begin(), title.end(), c, '_');
  }
  std::string jsonName = "../../../Questions/" + title + ".txt";

  std::ofstream outputJSON;
  outputJSON.open(jsonName);
  // should have to create the file so always should open
  if (!outputJSON.is_open())
  {
    std::cerr << "Error creating output file for JSON response" << std::endl;
    return;
  }

  // iterates through json response inserting key and value as pair into output file
  for (auto it = (*response).begin(); it != (*response).end(); ++it)
  {
    outputJSON << it.key() << ": " << it.value() << "\n";
  }
  outputJSON.close();
}