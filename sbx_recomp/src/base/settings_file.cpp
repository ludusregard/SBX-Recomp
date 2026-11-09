// The settings file (settings_file.h).
//
// Lines are matched by their key, not parsed as TOML: the SDK's reader
// (toml++) does the parsing at startup, and this only has to find a
// setting's line and swap its value. A key line is `name = value` before the
// first [table]; tables (the SDK's [log.levels]) are kept untouched after the
// settings.

#include "base/settings_file.h"

#include "base/log.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <optional>
#include <system_error>

namespace settings_file {
namespace {

std::filesystem::path g_path;

std::string_view Trim(std::string_view text) {
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
    text.remove_prefix(1);
  }
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
    text.remove_suffix(1);
  }
  return text;
}

bool IsKeyChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
}

// A line's parts, if it's `name = value` (with an optional `# comment`).
struct KeyLine {
  std::string indent;
  std::string name;
  std::string comment;  // from the '#' on, or empty
};

// Where a value's trailing comment starts in `value` (the text after '='),
// skipping a quoted string's contents. npos if there's none.
size_t CommentStart(std::string_view value) {
  size_t i = 0;
  while (i < value.size() && std::isspace(static_cast<unsigned char>(value[i]))) {
    i++;
  }
  if (i < value.size() && (value[i] == '"' || value[i] == '\'')) {
    char quote = value[i++];
    while (i < value.size() && value[i] != quote) {
      if (quote == '"' && value[i] == '\\') {
        i++;  // the escaped character
      }
      i++;
    }
    i++;  // the closing quote
  }
  size_t hash = value.find('#', std::min(i, value.size()));
  return hash;
}

std::optional<KeyLine> ParseKeyLine(std::string_view line) {
  size_t start = 0;
  while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) {
    start++;
  }
  size_t end = start;
  while (end < line.size() && IsKeyChar(line[end])) {
    end++;
  }
  if (end == start) {
    return std::nullopt;
  }
  size_t equals = end;
  while (equals < line.size() && (line[equals] == ' ' || line[equals] == '\t')) {
    equals++;
  }
  if (equals >= line.size() || line[equals] != '=') {
    return std::nullopt;
  }
  KeyLine key;
  key.indent = std::string(line.substr(0, start));
  key.name = std::string(line.substr(start, end - start));
  std::string_view value = line.substr(equals + 1);
  if (size_t hash = CommentStart(value); hash != std::string_view::npos) {
    key.comment = std::string(value.substr(hash));
  }
  return key;
}

bool IsTableHeader(std::string_view line) {
  line = Trim(line);
  return !line.empty() && line.front() == '[';
}

std::string KeyLineText(const std::string& indent, const Entry& entry,
                        const std::string& comment) {
  std::string line = indent + entry.name + " = " + entry.value;
  if (!comment.empty()) {
    line += "  " + comment;
  }
  return line;
}

const char* kNewFileHeader[] = {
    "# Shutoko Battle X settings, loaded at every start.",
    "# Written by the Save button in the F4 menu; safe to edit by hand while the",
    "# game is closed. One setting per line: name = value (strings in quotes).",
    "# Launch options (--name=value) and REX_<NAME> variables win over this file.",
};

}  // namespace

void SetPath(const std::filesystem::path& path) {
  g_path = path;
}

const std::filesystem::path& Path() {
  return g_path;
}

std::string TOMLValue(rex::cvar::FlagType type, std::string_view value) {
  using rex::cvar::FlagType;
  switch (type) {
    case FlagType::Boolean:
      return value == "true" || value == "1" ? "true" : "false";
    case FlagType::Int32:
    case FlagType::Int64:
    case FlagType::Uint32:
    case FlagType::Uint64:
      if (!value.empty() &&
          std::all_of(value.begin() + (value.front() == '-' ? 1 : 0), value.end(),
                      [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        return std::string(value);
      }
      return "0";
    case FlagType::Double: {
      std::string number(value);
      if (number.empty()) {
        return "0.0";
      }
      // TOML floats need a fraction or an exponent ("2" would read as an
      // integer, which the SDK's reader accepts too, but be exact).
      if (number.find_first_of(".eEni") == std::string::npos) {
        number += ".0";
      }
      return number;
    }
    default: {
      // A basic string, escaped.
      std::string text = "\"";
      for (char c : value) {
        switch (c) {
          case '"':
            text += "\\\"";
            break;
          case '\\':
            text += "\\\\";
            break;
          case '\n':
            text += "\\n";
            break;
          case '\r':
            text += "\\r";
            break;
          case '\t':
            text += "\\t";
            break;
          default:
            if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f) {
              char escaped[8];
              std::snprintf(escaped, sizeof(escaped), "\\u%04X", unsigned(c));
              text += escaped;
            } else {
              text += c;
            }
            break;
        }
      }
      text += '"';
      return text;
    }
  }
}

bool Save(const std::vector<Entry>& entries, std::string& error) {
  error.clear();
  if (g_path.empty()) {
    error = "no settings file path";
    return false;
  }

  // The current file, line by line (CR stripped).
  std::vector<std::string> lines;
  bool existed = false;
  {
    std::ifstream file(g_path, std::ios::binary);
    if (file) {
      existed = true;
      std::string line;
      while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }
        lines.push_back(std::move(line));
      }
    }
  }

  std::map<std::string, const Entry*> pending;
  for (const Entry& entry : entries) {
    pending[entry.name] = &entry;
  }

  // Settings already in the file: their values replaced in place.
  size_t first_table = lines.size();
  for (size_t i = 0; i < lines.size(); i++) {
    if (IsTableHeader(lines[i])) {
      first_table = i;
      break;
    }
    std::optional<KeyLine> key = ParseKeyLine(lines[i]);
    if (!key) {
      continue;
    }
    auto it = pending.find(key->name);
    if (it == pending.end()) {
      continue;
    }
    lines[i] = KeyLineText(key->indent, *it->second, key->comment);
    pending.erase(it);
  }

  // New settings, by section, before the first table. Ones at their default
  // add nothing.
  std::map<std::string, std::vector<const Entry*>> sections;
  for (const auto& [name, entry] : pending) {
    if (!entry->is_default) {
      sections[entry->section].push_back(entry);
    }
  }
  // A section whose comment is already in the file gets its new settings
  // under that comment, after the settings that follow it. The last ones
  // first, so earlier positions stay put.
  std::vector<std::pair<size_t, std::string>> in_place;
  for (const auto& [section, section_entries] : sections) {
    std::string header = "# " + (section.empty() ? std::string("Other") : section);
    for (size_t i = 0; i < first_table; i++) {
      if (Trim(lines[i]) == header) {
        size_t end = i + 1;
        while (end < first_table && ParseKeyLine(lines[end])) {
          end++;
        }
        in_place.emplace_back(end, section);
        break;
      }
    }
  }
  std::sort(in_place.rbegin(), in_place.rend());
  for (const auto& [position, section] : in_place) {
    std::vector<std::string> section_lines;
    for (const Entry* entry : sections[section]) {
      section_lines.push_back(KeyLineText("", *entry, ""));
    }
    lines.insert(lines.begin() + std::ptrdiff_t(position), section_lines.begin(),
                 section_lines.end());
    first_table += section_lines.size();
    sections.erase(section);
  }

  std::vector<std::string> added;
  if (!existed) {
    added.assign(std::begin(kNewFileHeader), std::end(kNewFileHeader));
  }
  for (const auto& [section, section_entries] : sections) {
    // A blank line between sections, and after what comes before.
    const std::string* previous = !added.empty()   ? &added.back()
                                  : first_table > 0 ? &lines[first_table - 1]
                                                    : nullptr;
    if (previous && !Trim(*previous).empty()) {
      added.push_back("");
    }
    added.push_back("# " + (section.empty() ? std::string("Other") : section));
    for (const Entry* entry : section_entries) {
      added.push_back(KeyLineText("", *entry, ""));
    }
  }
  if (!added.empty() && first_table < lines.size()) {
    added.push_back("");  // a gap before the tables
  }
  lines.insert(lines.begin() + std::ptrdiff_t(first_table), added.begin(), added.end());

  // Written beside the file, then moved over it.
  std::filesystem::path temporary = g_path;
  temporary += ".tmp";
  {
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    if (!file) {
      error = "couldn't create " + temporary.filename().string();
      return false;
    }
    for (const std::string& line : lines) {
      file << line << "\r\n";
    }
    file.flush();
    if (!file) {
      error = "couldn't write " + temporary.filename().string();
      return false;
    }
  }
  std::error_code code;
  std::filesystem::rename(temporary, g_path, code);
  if (code) {
    // Some file systems won't replace in a rename: copy over it instead.
    std::error_code copy_code;
    std::filesystem::copy_file(temporary, g_path,
                               std::filesystem::copy_options::overwrite_existing, copy_code);
    std::filesystem::remove(temporary, code);
    if (copy_code) {
      error = "couldn't replace " + g_path.filename().string() + ": " + copy_code.message();
      return false;
    }
  }
  ITC_LOG_INFO("settings: saved {} setting(s) to {}", entries.size(),
               g_path.filename().string());
  return true;
}

}  // namespace settings_file
