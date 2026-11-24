#ifndef ES_REGEX_ADAPTER_H
#define ES_REGEX_ADAPTER_H
#include "third_party/tre/lib/tre.h"
#include <string>

/* Byte-oriented GNU AWK compatibility layer for EventScripts commands. */
class EsRegex {
public:
    EsRegex();
    ~EsRegex();
    bool compile(const char *pattern);
    int match(const char *text, regmatch_t *groups, size_t count) const;
    int search(const char *text, int start, int range) const;
private:
    EsRegex(const EsRegex &);
    EsRegex &operator=(const EsRegex &);
    regex_t re_;
    std::string pattern_;
    int flags_;
    bool compiled_;
    bool backref_;
};
#endif
