#include <xzero/http/v1/HttpParser.h>
#include <xzero/http/HttpListener.h>
#include <xzero/Buffer.h>
#include <vector>
#include <gtest/gtest.h>

using namespace xzero;
using namespace xzero::http1;

class HttpParserCallbacks : public xzero::HttpListener {  // {{{
 public:
  HttpParserCallbacks() = default;

  bool onMessageBegin(const BufferRef& method, const BufferRef& entity,
                      int versionMajor, int versionMinor) override;
  bool onMessageBegin(int versionMajor, int versionMinor, int code,
                      const BufferRef& text) override;
  bool onMessageBegin() override;
  bool onMessageHeader(const BufferRef& name, const BufferRef& value) override;
  bool onMessageHeaderEnd() override;
  bool onMessageContent(const BufferRef& chunk) override;
  bool onMessageEnd() override;
  void onProtocolError(const BufferRef& chunk, size_t offset) override;

 public:
  std::function<bool(const BufferRef& method, const BufferRef& entity,
                     int versionMajor, int versionMinor)> requestStart;
  std::function<bool(int versionMajor, int versionMinor, int code,
                     const BufferRef& text)> responseStart;
  std::function<bool()> messageStart;
  std::function<bool(const BufferRef& name, const BufferRef& value)> header;
  std::function<bool()> headerEnd;
  std::function<bool(const BufferRef& chunk)> content;
  std::function<bool()> end;
  std::function<void(const BufferRef& chunk, size_t offset)> protocolError;
};

bool HttpParserCallbacks::onMessageBegin(const BufferRef& method,
                                         const BufferRef& entity,
                                         int versionMajor, int versionMinor) {
  if (requestStart)
    return requestStart(method, entity, versionMajor, versionMinor);
  else
    return true;
}

bool HttpParserCallbacks::onMessageBegin(int versionMajor, int versionMinor,
                                         int code, const BufferRef& text) {
  if (responseStart)
    return responseStart(versionMajor, versionMinor, code, text);
  else
    return true;
}

bool HttpParserCallbacks::onMessageBegin() {
  if (messageStart)
    return messageStart();
  else
    return true;
}

bool HttpParserCallbacks::onMessageHeader(const BufferRef& name,
                                          const BufferRef& value) {
  if (header)
    return header(name, value);
  else
    return true;
}

bool HttpParserCallbacks::onMessageHeaderEnd() {
  if (headerEnd)
    return headerEnd();
  else
    return true;
}

bool HttpParserCallbacks::onMessageContent(const BufferRef& chunk) {
  if (content)
    return content(chunk);
  else
    return true;
}

bool HttpParserCallbacks::onMessageEnd() {
  if (end)
    return end();
  else
    return true;
}

void HttpParserCallbacks::onProtocolError(const BufferRef& chunk,
                                          size_t offset) {
  if (protocolError) protocolError(chunk, offset);
}
// }}}
class HttpParserListener : public xzero::HttpListener {  // {{{
 public:
  HttpParserListener();

  bool onMessageBegin(const BufferRef& method, const BufferRef& entity,
                      int versionMajor, int versionMinor) override;
  bool onMessageBegin(int versionMajor, int versionMinor, int code,
                      const BufferRef& text) override;
  bool onMessageBegin() override;
  bool onMessageHeader(const BufferRef& name, const BufferRef& value) override;
  bool onMessageHeaderEnd() override;
  bool onMessageContent(const BufferRef& chunk) override;
  bool onMessageEnd() override;
  void onProtocolError(const BufferRef& chunk, size_t offset) override;

 public:
  std::string method;
  std::string entity;
  int versionMajor;
  int versionMinor;
  int statusCode;
  std::string statusReason;
  std::vector<std::pair<std::string, std::string>> headers;
  Buffer body;
  std::string errorMessage;
  int errorOffset;

  bool messageBegin;
  bool headerEnd;
  bool messageEnd;
};

HttpParserListener::HttpParserListener()
    : method(),
      entity(),
      versionMajor(-1),
      versionMinor(-1),
      statusCode(-1),
      statusReason(),
      headers(),
      errorMessage(),
      errorOffset(-1),
      messageBegin(false),
      headerEnd(false),
      messageEnd(false) {}

bool HttpParserListener::onMessageBegin(const BufferRef& method,
                                        const BufferRef& entity,
                                        int versionMajor, int versionMinor) {
  this->method = method.str();
  this->entity = entity.str();
  this->versionMajor = versionMajor;
  this->versionMinor = versionMinor;

  return true;
}

bool HttpParserListener::onMessageBegin(int versionMajor, int versionMinor,
                                        int code, const BufferRef& text) {
  this->versionMajor = versionMajor;
  this->versionMinor = versionMinor;
  this->statusCode = code;
  this->statusReason = text.str();

  return true;
}

bool HttpParserListener::onMessageBegin() {
  messageBegin = true;
  return true;
}

bool HttpParserListener::onMessageHeader(const BufferRef& name,
                                         const BufferRef& value) {
  headers.push_back(std::make_pair(name.str(), value.str()));
  return true;
}

bool HttpParserListener::onMessageHeaderEnd() {
  headerEnd = true;
  return true;
}

bool HttpParserListener::onMessageContent(const BufferRef& chunk) {
  body += chunk;
  return true;
}

bool HttpParserListener::onMessageEnd() {
  messageEnd = true;
  return true;
}

void HttpParserListener::onProtocolError(const BufferRef& chunk,
                                         size_t offset) {
  errorMessage = chunk;
  errorOffset = offset;
}
// }}}

TEST(HttpParser, requestLine1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/0.9\r\n\r\n");

  ASSERT_EQ("GET", listener.method);
  ASSERT_EQ("/", listener.entity);
  ASSERT_EQ(0, listener.versionMajor);
  ASSERT_EQ(9, listener.versionMinor);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(HttpParser, requestLine2) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("HEAD /foo?bar HTTP/1.0\r\n\r\n");

  ASSERT_EQ("HEAD", listener.method);
  ASSERT_EQ("/foo?bar", listener.entity);
  ASSERT_EQ(1, listener.versionMajor);
  ASSERT_EQ(0, listener.versionMinor);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(HttpParser, requestWithHeaders) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Foo: the foo\r\n"
      "X-Bar: the bar\r\n"
      "\r\n");

  ASSERT_EQ("GET", listener.method);
  ASSERT_EQ("/", listener.entity);
  ASSERT_EQ(0, listener.versionMajor);
  ASSERT_EQ(9, listener.versionMinor);
  ASSERT_EQ(2, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());

  ASSERT_EQ("Foo", listener.headers[0].first);
  ASSERT_EQ("the foo", listener.headers[0].second);

  ASSERT_EQ("X-Bar", listener.headers[1].first);
  ASSERT_EQ("the bar", listener.headers[1].second);
}

TEST(HttpParser, requestWithHeadersAndBody) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Foo: the foo\r\n"
      "X-Bar: the bar\r\n"
      "Content-Length: 6\r\n"
      "\r\n"
      "123456");

  ASSERT_EQ("123456", listener.body);
}

// no chunks except the EOS-chunk
TEST(HttpParser, requestWithHeadersAndBodyChunked1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "0\r\n"
      "\r\n");

  ASSERT_EQ("", listener.body);
}

// exactly one data chunk
TEST(HttpParser, requestWithHeadersAndBodyChunked2) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"

      "6\r\n"
      "123456"
      "\r\n"

      "0\r\n"
      "\r\n");

  ASSERT_EQ("123456", listener.body);
}

// more than one data chunk
TEST(HttpParser, requestWithHeadersAndBodyChunked3) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"

      "6\r\n"
      "123456"
      "\r\n"

      "6\r\n"
      "123456"
      "\r\n"

      "0\r\n"
      "\r\n");

  ASSERT_EQ("123456123456", listener.body);
}

// first chunk is missing CR LR
TEST(HttpParser, requestWithHeadersAndBodyChunked_invalid1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  size_t n = parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"

      "6\r\n"
      "123456"
      //"\r\n" // should bailout here

      "0\r\n"
      "\r\n");

  ASSERT_EQ(55, n);
  ASSERT_EQ(55, listener.errorOffset);
}

TEST(HttpParser, pipelined1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET /foo HTTP/1.1\r\n\r\n"
                       "HEAD /bar HTTP/0.9\r\n\r\n");

  ASSERT_EQ("HEAD", listener.method);
  ASSERT_EQ("/bar", listener.entity);
  ASSERT_EQ(0, listener.versionMajor);
  ASSERT_EQ(9, listener.versionMinor);
}

