#include <cassert>
#include <cstdio>
#include <string>

#include "Opds2Parser.h"
#include "OpdsFeedParser.h"
#include "OpdsPublicationDoc.h"

namespace {
std::string atomDescription(const std::string& content) {
  const std::string feed =
      R"(<feed xmlns="http://www.w3.org/2005/Atom"><title>Books</title><entry><title>Book</title>)"
      R"(<link rel="http://opds-spec.org/acquisition" type="application/epub+zip" href="/book.epub"/>)"
      R"(<content type="text">)" +
      content + "</content></entry></feed>";
  OpdsFeedParser parser;
  parser.write(reinterpret_cast<const uint8_t*>(feed.data()), feed.size());
  parser.flush();
  assert(!parser.error());
  const auto entries = parser.takeEntries();
  assert(entries.size() == 1);
  return entries[0].description;
}

std::string jsonFeedDescription(const std::string& content, const std::string& rest = "") {
  const std::string feed =
      R"({"metadata":{"title":"Books"},"publications":[{"metadata":{"title":"Book","description":")" + content + "\"" +
      rest + R"(},"links":[{"rel":"http://opds-spec.org/acquisition","type":"application/epub+zip","href":"/book.epub"}]}]})";
  Opds2Parser parser;
  parser.write(reinterpret_cast<const uint8_t*>(feed.data()), feed.size());
  parser.flush();
  assert(!parser.error());
  assert(parser.getEntries().size() == 1);
  return parser.getEntries()[0].description;
}

// `rest` follows the description inside the same metadata object.
std::string publicationDescription(const std::string& content, const std::string& rest = "") {
  const std::string doc = R"({"metadata":{"title":"Book","description":")" + content + "\"" + rest + "}}";
  OpdsPublication publication;
  assert(parseOpdsPublicationDoc(doc.data(), doc.size(), publication, nullptr));
  return publication.description;
}
}  // namespace

int main() {
#if FREEINK_DEVICE_X4PRO
  constexpr size_t CAP = 4096;
#else
  constexpr size_t CAP = 500;
#endif
  static_assert(OpdsLimits::MAX_DESCRIPTION_CHARS == CAP);

  // Kept whole below the cap, cut at it above.
  assert(atomDescription(std::string(CAP - 1, 'a')) == std::string(CAP - 1, 'a'));
  assert(atomDescription(std::string(CAP + 100, 'a')) == std::string(CAP, 'a'));

#if FREEINK_DEVICE_X4PRO
  // A cap that falls inside a multibyte character drops that character whole.
  for (const std::string& wide :
       {std::string("\xC3\xA9"), std::string("\xE2\x80\x94"), std::string("\xF0\x9F\x93\x9A")}) {
    for (size_t kept = 1; kept < wide.size(); kept++) {
      const std::string head(CAP - kept, 'a');
      assert(atomDescription(head + wide + "tail") == head);
    }
    const std::string head(CAP - wide.size(), 'a');
    assert(atomDescription(head + wide + "tail") == head + wide);
  }
#endif

  // OPDS 2 strings are bounded by the JSON token buffer, whatever the cap.
  constexpr size_t TOKEN = StreamingJsonParser::TOKEN_BUF_SIZE - 1;
  const std::string longText(TOKEN + 500, 'a');
#if FREEINK_DEVICE_X4PRO
  assert(jsonFeedDescription(std::string(TOKEN, 'a')) == std::string(TOKEN, 'a'));
  assert(jsonFeedDescription(longText) == std::string(TOKEN, 'a'));
  assert(publicationDescription(longText) == std::string(TOKEN, 'a'));
  const std::string head(TOKEN - 1, 'a');
  assert(jsonFeedDescription(head + "\xC3\xA9tail") == head);
  assert(publicationDescription(head + "\xC3\xA9tail") == head);
  assert(publicationDescription(head + "\\u00e9tail") == head);
  // A key too long to report leaves the previous one pending; its value must not replace the description.
  const std::string unreported = ",\"" + longText + "\":\"" + longText + "\"";
  assert(jsonFeedDescription("blurb", unreported) == "blurb");
  assert(publicationDescription("blurb", unreported) == "blurb");
#else
  assert(jsonFeedDescription(std::string(CAP + 100, 'a')) == std::string(CAP, 'a'));
  assert(jsonFeedDescription(longText).empty());
  assert(publicationDescription(longText).empty());
#endif
  std::puts("OPDS description tests passed");
}
