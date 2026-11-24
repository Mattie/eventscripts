#include "es_regex_adapter.h"
#include <limits.h>
#include <string.h>
#include <vector>

EsRegex::EsRegex() : flags_(REG_EXTENDED), compiled_(false), backref_(false) {}
EsRegex::~EsRegex() { if (compiled_) tre_regfree(&re_); }

bool EsRegex::compile(const char *p)
{
    if (compiled_) { tre_regfree(&re_); compiled_ = false; }
    pattern_.clear(); backref_ = false;
    bool strict = strstr(p, "\\`") || strstr(p, "\\'");
    if (strict && (strchr(p, '^') || strchr(p, '$'))) return false;
    flags_ = REG_EXTENDED | (strict ? 0 : REG_NEWLINE);
    bool in_class = false, first = false, hyphen = false;
    int group = 0;
    std::vector<int> open;
    for (size_t i = 0; p[i]; ++i) {
        char c = p[i];
        if (in_class && strncmp(p + i, "[:blank:]", 9) == 0) {
            pattern_ += " \t"; i += 8; first = false; continue;
        }
        if (c == '\\' && p[i+1]) {
            char d = p[++i];
            if (in_class && d == '-') {
                if ((i >= 2 && p[i-2] == '-') || p[i+1] == '-') return false;
                if (first) pattern_ += '-'; else hyphen = true;
            } else if (in_class && d == ']' && first) pattern_ += ']';
            else if (in_class && (d == 'd' || d == 'D' || (d >= '0' && d <= '9'))) pattern_ += d;
            else if (in_class) { pattern_ += '\\'; pattern_ += d; }
            else if (d >= '1' && d <= '9') {
                for (size_t j = 0; j < open.size(); ++j) if (open[j] == d-'0') return false;
                backref_ = true; pattern_ += '\\'; pattern_ += d;
            } else if (d == '`') pattern_ += '^';
            else if (d == '\'') pattern_ += '$';
            else if (((d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z')) && !strchr("bBwWsS", d)) pattern_ += d;
            else if (d == '0') pattern_ += '0';
            else { pattern_ += '\\'; pattern_ += d; }
            if (in_class) first = false;
            continue;
        }
        if (in_class) {
            if (c == ']' && !first) { if (hyphen) pattern_ += '-'; hyphen = false; in_class = false; }
            else if (!(first && c == '^')) first = false;
            pattern_ += c; continue;
        }
        if (c == '[') { in_class = true; first = true; pattern_ += c; }
        else if (c == '{' || c == '}') { pattern_ += '\\'; pattern_ += c; }
        else if (c == '.' && !strict) pattern_.append("[\001-\377]", 5);
        else if (c == '(') { open.push_back(++group); pattern_ += c; if (p[i+1] == '?') { pattern_ += "\\?"; ++i; } }
        else if (c == ')') { if (!open.empty()) open.pop_back(); pattern_ += c; }
        else if (strchr("*+?", c) && !pattern_.empty() && strchr("*+?", pattern_[pattern_.size()-1])) {
            size_t slashes = 0, pos = pattern_.size() - 1;
            while (pos > 0 && pattern_[pos-1] == '\\') { ++slashes; --pos; }
            if (slashes % 2) pattern_ += c;
            else {
                char prev = pattern_[pattern_.size()-1];
                pattern_[pattern_.size()-1] = prev == c && c == '?' ? '?' : prev == c && c == '+' ? '+' : '*';
            }
        } else pattern_ += c;
    }
    compiled_ = tre_regcomp(&re_, pattern_.c_str(), flags_) == 0;
    return compiled_;
}

int EsRegex::match(const char *text, regmatch_t *groups, size_t count) const
{
    regmatch_t one;
    if (!compiled_) return -2;
    if (!groups || count == 0) { groups = &one; count = 1; }
    int rc = tre_regexec(&re_, text, count, groups, 0);
    if (rc == REG_NOMATCH || (!rc && groups[0].rm_so != 0)) return -1;
    return rc ? -2 : groups[0].rm_eo;
}

int EsRegex::search(const char *text, int start, int range) const
{
    size_t length = strlen(text);
    if (!compiled_ || backref_ || length > INT_MAX) return -2;
    int len = (int)length;
    if (start < 0 || start > len) return -1;
    int end = range > 0 && range > len-start ? len : range < 0 && range < -start ? 0 : start+range;
    regex_t context;
    std::string wrapped("[\001-\377](", 6); wrapped += pattern_; wrapped += ')';
    if (tre_regcomp(&context, wrapped.c_str(), flags_) != 0) return -2;
    int found = -1;
    regmatch_t m;
    if (range >= 0) {
        int prefix = start > 0;
        int eflags = start > 1 && (!(flags_ & REG_NEWLINE) || text[start-2] != '\n') ? REG_NOTBOL : 0;
        int rc = tre_regexec(prefix ? &context : &re_, text + start - prefix, 1, &m, eflags);
        if (!rc && m.rm_so <= end-start) found = start + m.rm_so;
        else if (rc && rc != REG_NOMATCH) found = -2;
    } else {
        for (int pos = start; pos >= end; --pos) {
            int prefix = pos > 0;
            int eflags = pos > 1 && (!(flags_ & REG_NEWLINE) || text[pos-2] != '\n') ? REG_NOTBOL : 0;
            int rc = tre_regexec(prefix ? &context : &re_, text + pos - prefix, 1, &m, eflags);
            if (!rc && m.rm_so == 0) { found = pos; break; }
            if (rc && rc != REG_NOMATCH) { found = -2; break; }
        }
    }
    tre_regfree(&context);
    return found;
}
