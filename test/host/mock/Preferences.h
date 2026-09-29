#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <string>

class Preferences {
  std::map<std::string, int32_t> ints;
  std::map<std::string, uint32_t> uints;
  std::map<std::string, bool> bools;
  std::map<std::string, std::string> strs;
  std::map<std::string, std::string> blobs;

 public:
  bool begin(const char*, bool) { return true; }
  void end() {}
  // The real one logs an NVS error when a missing key is read, so callers ask
  // this first. The mock has to answer the same question.
  bool isKey(const char* k) {
    return ints.count(k) || uints.count(k) || bools.count(k) || strs.count(k) ||
           blobs.count(k);
  }
  int32_t getInt(const char* k, int32_t d = 0) {
    auto it = ints.find(k);
    return it == ints.end() ? d : it->second;
  }
  void putInt(const char* k, int32_t v) { ints[k] = v; }
  uint32_t getUInt(const char* k, uint32_t d = 0) {
    auto it = uints.find(k);
    return it == uints.end() ? d : it->second;
  }
  void putUInt(const char* k, uint32_t v) { uints[k] = v; }
  bool getBool(const char* k, bool d = false) {
    auto it = bools.find(k);
    return it == bools.end() ? d : it->second;
  }
  void putBool(const char* k, bool v) { bools[k] = v; }

  // Blobs, for the saved game boards. Same contract as the ESP32 version: a
  // length of 0 means the key is absent, and a short read returns 0.
  size_t putBytes(const char* k, const void* v, size_t n) {
    blobs[k] = std::string((const char*)v, n);
    return n;
  }
  size_t getBytesLength(const char* k) {
    auto it = blobs.find(k);
    return it == blobs.end() ? 0 : it->second.size();
  }
  size_t getBytes(const char* k, void* out, size_t max) {
    auto it = blobs.find(k);
    if (it == blobs.end() || it->second.size() > max) return 0;
    memcpy(out, it->second.data(), it->second.size());
    return it->second.size();
  }
  size_t getString(const char* k, char* out, size_t max) {
    auto it = strs.find(k);
    if (it == strs.end() || max == 0) {
      if (max) out[0] = 0;
      return 0;
    }
    const size_t n = it->second.size() < max - 1 ? it->second.size() : max - 1;
    memcpy(out, it->second.data(), n);
    out[n] = 0;
    return n;
  }
  void putString(const char* k, const char* v) { strs[k] = v; }
  // The real one returns false for a key that was not there; nothing in the
  // firmware checks, and settings deliberately removes keys it may never have
  // written.
  bool remove(const char* k) {
    const size_t n =
        ints.erase(k) + uints.erase(k) + bools.erase(k) + strs.erase(k) + blobs.erase(k);
    return n > 0;
  }

  // Everything, as one string and back, for the web emulator to keep in the
  // browser between visits. One entry a line: type, key, value as hex.
  std::string serialize() const {
    std::string out;
    auto hex = [&](const std::string& v) {
      static const char* H = "0123456789abcdef";
      for (unsigned char ch : v) {
        out += H[ch >> 4];
        out += H[ch & 15];
      }
    };
    auto line = [&](char t, const std::string& k, const std::string& v) {
      out += t;
      out += '\t';
      out += k;
      out += '\t';
      hex(v);
      out += '\n';
    };
    for (auto& e : ints) line('i', e.first, std::to_string(e.second));
    for (auto& e : uints) line('u', e.first, std::to_string(e.second));
    for (auto& e : bools) line('b', e.first, e.second ? "1" : "0");
    for (auto& e : strs) line('s', e.first, e.second);
    for (auto& e : blobs) line('x', e.first, e.second);
    return out;
  }
  void deserialize(const std::string& in) {
    size_t p = 0;
    while (p < in.size()) {
      const size_t eol = in.find('\n', p);
      const std::string l = in.substr(p, eol == std::string::npos ? std::string::npos : eol - p);
      p = eol == std::string::npos ? in.size() : eol + 1;
      const size_t a = l.find('\t'), b = a == std::string::npos ? a : l.find('\t', a + 1);
      if (a != 1 || b == std::string::npos) continue;
      const std::string k = l.substr(2, b - 2), hx = l.substr(b + 1);
      std::string v;
      for (size_t i = 0; i + 1 < hx.size(); i += 2)
        v += (char)std::stoi(hx.substr(i, 2), nullptr, 16);
      switch (l[0]) {
        case 'i': ints[k] = (int32_t)std::stol(v); break;
        case 'u': uints[k] = (uint32_t)std::stoul(v); break;
        case 'b': bools[k] = v == "1"; break;
        case 's': strs[k] = v; break;
        case 'x': blobs[k] = v; break;
        default: break;
      }
    }
  }
};
