#include "audio/music/metadata.h"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>

namespace audio::music {

namespace {

struct Attribute {
  std::string name, value;
};

struct Element {
  std::string name;
  std::vector<Attribute> attributes;
  int depth = 0;  // 0 the root
  int line = 0;
};

class Reader {
 public:
  explicit Reader(std::string_view text) : text_(text) {
    if (text_.substr(0, 3) == "\xEF\xBB\xBF") {
      at_ = 3;  // the UTF-8 byte order mark
    }
  }

  // Reads every element; false and `error` if the file is malformed.
  bool Read(std::vector<Element>* elements, std::string* error) {
    std::vector<std::string> open;
    bool had_root = false;
    while (true) {
      const size_t lt = text_.find('<', at_);
      Skipped(lt == std::string_view::npos ? text_.size() : lt);
      if (lt == std::string_view::npos) {
        break;
      }
      at_ = lt;
      if (Starts("<?")) {
        if (!SkipPast("?>", error)) return false;
      } else if (Starts("<!--")) {
        if (!SkipPast("-->", error)) return false;
      } else if (Starts("<!")) {
        if (Starts("<![CDATA[")) {
          return Fail("CDATA isn't used here", error);
        }
        if (!SkipPast(">", error)) return false;  // <!DOCTYPE ...>
      } else if (Starts("</")) {
        at_ += 2;
        const std::string name = Name();
        SkipSpace();
        if (!Expect('>', error)) return false;
        if (open.empty() || open.back() != name) {
          return Fail("</" + name + "> doesn't close an open element", error);
        }
        open.pop_back();
      } else {
        at_++;
        Element e;
        e.line = line_;
        e.depth = int(open.size());
        e.name = Name();
        if (e.name.empty()) {
          return Fail("an element without a name", error);
        }
        if (open.empty() && had_root) {
          return Fail("a second root element <" + e.name + ">", error);
        }
        had_root = true;
        bool closed = false;
        while (true) {
          SkipSpace();
          if (Starts("/>")) {
            at_ += 2;
            closed = true;
            break;
          }
          if (Starts(">")) {
            at_++;
            break;
          }
          Attribute a;
          a.name = Name();
          if (a.name.empty()) {
            return Fail("unexpected text in <" + e.name + ">", error);
          }
          SkipSpace();
          if (!Expect('=', error)) return false;
          SkipSpace();
          if (!Value(&a.value, error)) return false;
          e.attributes.push_back(std::move(a));
        }
        if (!closed) {
          open.push_back(e.name);
        }
        elements->push_back(std::move(e));
      }
    }
    if (!open.empty()) {
      return Fail("<" + open.back() + "> is never closed", error);
    }
    if (!had_root) {
      return Fail("no elements", error);
    }
    return true;
  }

 private:
  bool Starts(std::string_view s) const { return text_.substr(at_, s.size()) == s; }

  void Skipped(size_t to) {
    for (; at_ < to; at_++) {
      if (text_[at_] == '\n') line_++;
    }
  }

  bool SkipPast(std::string_view end, std::string* error) {
    const size_t found = text_.find(end, at_);
    if (found == std::string_view::npos) {
      return Fail("\"" + std::string(end) + "\" is missing", error);
    }
    Skipped(found + end.size());
    return true;
  }

  void SkipSpace() {
    while (at_ < text_.size() && std::isspace(uint8_t(text_[at_]))) {
      if (text_[at_] == '\n') line_++;
      at_++;
    }
  }

  std::string Name() {
    const size_t start = at_;
    while (at_ < text_.size()) {
      const char c = text_[at_];
      if (std::isalnum(uint8_t(c)) || c == '_' || c == '-' || c == ':' || c == '.' ||
          uint8_t(c) >= 0x80) {
        at_++;
      } else {
        break;
      }
    }
    return std::string(text_.substr(start, at_ - start));
  }

  bool Expect(char c, std::string* error) {
    if (at_ < text_.size() && text_[at_] == c) {
      at_++;
      return true;
    }
    return Fail(std::string("expected '") + c + "'", error);
  }

  bool Value(std::string* out, std::string* error) {
    if (at_ >= text_.size() || (text_[at_] != '"' && text_[at_] != '\'')) {
      return Fail("an attribute value must be quoted", error);
    }
    const char quote = text_[at_++];
    while (at_ < text_.size() && text_[at_] != quote) {
      const char c = text_[at_];
      if (c == '<') {
        return Fail("'<' in an attribute value (write &lt;)", error);
      }
      if (c == '\n') line_++;
      if (c == '&') {
        if (!Entity(out, error)) return false;
        continue;
      }
      out->push_back(c);
      at_++;
    }
    if (at_ >= text_.size()) {
      return Fail("an attribute value is never closed", error);
    }
    at_++;
    return true;
  }

  bool Entity(std::string* out, std::string* error) {
    const size_t semi = text_.find(';', at_);
    if (semi == std::string_view::npos || semi - at_ > 10) {
      return Fail("'&' must start an entity (write &amp;)", error);
    }
    const std::string_view name = text_.substr(at_ + 1, semi - at_ - 1);
    at_ = semi + 1;
    if (name == "amp") out->push_back('&');
    else if (name == "lt") out->push_back('<');
    else if (name == "gt") out->push_back('>');
    else if (name == "quot") out->push_back('"');
    else if (name == "apos") out->push_back('\'');
    else if (!name.empty() && name[0] == '#') {
      uint32_t code = 0;
      const bool hex = name.size() > 1 && (name[1] == 'x' || name[1] == 'X');
      const std::string_view digits = name.substr(hex ? 2 : 1);
      auto [end, ec] =
          std::from_chars(digits.data(), digits.data() + digits.size(), code, hex ? 16 : 10);
      if (digits.empty() || ec != std::errc() || end != digits.data() + digits.size() ||
          code == 0 || code > 0x10FFFF) {
        return Fail("a bad character reference &" + std::string(name) + ";", error);
      }
      // UTF-8.
      if (code < 0x80) {
        out->push_back(char(code));
      } else if (code < 0x800) {
        out->push_back(char(0xC0 | (code >> 6)));
        out->push_back(char(0x80 | (code & 0x3F)));
      } else if (code < 0x10000) {
        out->push_back(char(0xE0 | (code >> 12)));
        out->push_back(char(0x80 | ((code >> 6) & 0x3F)));
        out->push_back(char(0x80 | (code & 0x3F)));
      } else {
        out->push_back(char(0xF0 | (code >> 18)));
        out->push_back(char(0x80 | ((code >> 12) & 0x3F)));
        out->push_back(char(0x80 | ((code >> 6) & 0x3F)));
        out->push_back(char(0x80 | (code & 0x3F)));
      }
    } else {
      return Fail("an unknown entity &" + std::string(name) + ";", error);
    }
    return true;
  }

  bool Fail(const std::string& what, std::string* error) {
    *error = "line " + std::to_string(line_) + ": " + what;
    return false;
  }

  std::string_view text_;
  size_t at_ = 0;
  int line_ = 1;
};

// A decimal number ("0.5", "184.25"), or nullopt.
std::optional<double> Number(std::string_view s) {
  while (!s.empty() && std::isspace(uint8_t(s.front()))) s.remove_prefix(1);
  while (!s.empty() && std::isspace(uint8_t(s.back()))) s.remove_suffix(1);
  double v = 0;
  auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
  if (s.empty() || ec != std::errc() || end != s.data() + s.size() || !std::isfinite(v)) {
    return std::nullopt;
  }
  return v;
}

std::string At(int line) {
  return "line " + std::to_string(line) + ": ";
}

}  // namespace

bool SameName(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); i++) {
    if (std::tolower(uint8_t(a[i])) != std::tolower(uint8_t(b[i]))) {
      return false;
    }
  }
  return true;
}

std::optional<Metadata> ParseMetadata(std::string_view text, std::string* error,
                                      std::vector<std::string>* warnings) {
  std::vector<Element> elements;
  if (!Reader(text).Read(&elements, error)) {
    return std::nullopt;
  }
  const Element& root = elements.front();
  if (!SameName(root.name, "Music")) {
    *error = At(root.line) + "the root element must be <Music>, not <" + root.name + ">";
    return std::nullopt;
  }
  Metadata m;
  for (const Attribute& a : root.attributes) {
    if (SameName(a.name, "Category")) {
      m.category = a.value;
    } else if (SameName(a.name, "ReplaceChance")) {
      auto v = Number(a.value);
      if (!v || *v < 0 || *v > 1) {
        *error = At(root.line) + "ReplaceChance must be a number from 0 to 1, not \"" +
                 a.value + "\"";
        return std::nullopt;
      }
      m.replace_chance = v;
    } else {
      warnings->push_back(At(root.line) + "<Music> has no attribute " + a.name + "; ignored");
    }
  }
  for (size_t i = 1; i < elements.size(); i++) {
    const Element& e = elements[i];
    if (e.depth == 1 && SameName(e.name, "Game")) {
      GameEntry g;
      g.line = e.line;
      for (const Attribute& a : e.attributes) {
        if (SameName(a.name, "Name")) {
          g.name = a.value;
        } else if (SameName(a.name, "Weight")) {
          auto v = Number(a.value);
          if (!v || *v < 0) {
            *error = At(e.line) + "Weight must be a number of 0 or more, not \"" + a.value + "\"";
            return std::nullopt;
          }
          g.weight = *v;
        } else {
          warnings->push_back(At(e.line) + "<Game> has no attribute " + a.name + "; ignored");
        }
      }
      if (g.name.empty()) {
        warnings->push_back(At(e.line) + "a <Game> without a Name; skipped");
      } else {
        m.game.push_back(std::move(g));
      }
      continue;
    }
    if (e.depth != 1 || !SameName(e.name, "Track")) {
      warnings->push_back(At(e.line) + "<" + e.name + "> isn't used; ignored");
      continue;
    }
    TrackEntry t;
    t.line = e.line;
    bool bad = false;
    for (const Attribute& a : e.attributes) {
      if (SameName(a.name, "File")) {
        t.file = a.value;
      } else if (SameName(a.name, "Weight")) {
        auto v = Number(a.value);
        if (!v || *v < 0) {
          *error = At(e.line) + "Weight must be a number of 0 or more, not \"" + a.value + "\"";
          return std::nullopt;
        }
        t.weight = *v;
      } else if (SameName(a.name, "LoopIn") || SameName(a.name, "LoopOut")) {
        auto v = Number(a.value);
        if (!v || *v < 0) {
          *error = At(e.line) + a.name + " must be seconds (0 or more), not \"" + a.value + "\"";
          return std::nullopt;
        }
        (SameName(a.name, "LoopIn") ? t.loop_in : t.loop_out) = v;
      } else {
        warnings->push_back(At(e.line) + "<Track> has no attribute " + a.name + "; ignored");
      }
    }
    if (t.file.empty()) {
      warnings->push_back(At(e.line) + "a <Track> without a File; skipped");
      bad = true;
    }
    if (t.loop_in.has_value() != t.loop_out.has_value()) {
      warnings->push_back(At(e.line) + t.file +
                          ": LoopIn and LoopOut go together; the whole track loops");
      t.loop_in.reset();
      t.loop_out.reset();
    } else if (t.loop_in && *t.loop_in >= *t.loop_out) {
      warnings->push_back(At(e.line) + t.file +
                          ": LoopIn must be before LoopOut; the whole track loops");
      t.loop_in.reset();
      t.loop_out.reset();
    }
    if (!bad) {
      m.tracks.push_back(std::move(t));
    }
  }
  return m;
}

}  // namespace audio::music
