#include "Internet.hpp"

HttpDownloader::HttpDownloader(HINTERNET internet, const wchar_t* server)
  : serverConnection{ InternetConnect(internet, server, INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, NULL) }
{
}

bool HttpDownloader::tryDownloadImage(const wchar_t* url, std::vector<byte>& buffer)
{
  const wchar_t* acceptTypes[] = { L"image/png", NULL };
  constexpr DWORD httpRequestFlags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_SECURE;

  InternetHandle httpRequest{ HttpOpenRequest(serverConnection.get(), L"GET", url, nullptr, nullptr, acceptTypes, httpRequestFlags, NULL) };
  if (!httpRequest)
  {
    logError("HttpOpenRequest failed.");
    return false;
  }

  if (!HttpSendRequest(httpRequest.get(), nullptr, 0, nullptr, 0))
  {
    logError("HttpSendRequest failed.");
    return false;
  }

  DWORD statusCode = queryRequestNumber(httpRequest.get(), HTTP_QUERY_STATUS_CODE);
  if (statusCode != 200)
  {
    logError("Http request failed. Status code %lu.", statusCode);
    return false;
  }

  DWORD contentLength = queryRequestNumber(httpRequest.get(), HTTP_QUERY_CONTENT_LENGTH);
  if (contentLength == 0)
  {
    logError("Failed to get http request content length");
    return false;
  }

  buffer.resize(contentLength);
  uint64 contentBufferWriteOffset = 0;
  while (true)
  {
    DWORD bytesRead;
    if (!InternetReadFile(httpRequest.get(), buffer.data() + contentBufferWriteOffset, contentLength, &bytesRead))
    {
      if (contentBufferWriteOffset != contentLength)
      {
        logError("InternetReadFile failed.");
        return false;
      }
      else
      {
        break;
      }
    }

    if (bytesRead == 0)
    {
      break;
    }

    contentBufferWriteOffset += bytesRead;
  }

  return true;
}
