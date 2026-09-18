/* tractatus_data.c -- complete bilingual Tractatus proposition corpus.
 *
 * The generated include contains all 526 numbered propositions from Project
 * Gutenberg eBook #5740: the Ogden/Ramsey translation and original German.
 * The source edition is public domain in the United States.
 */
#include "tractatus.h"
#include <string.h>

#include "tractatus_corpus.inc"

const char *tr_text(const TrProp *prop, TrLanguage language)
{
    if (!prop) return NULL;
    return language == TR_GERMAN ? prop->german : prop->english;
}

const char *tr_seven(int i)
{
    static const char *numbers[] = {"1", "2", "3", "4", "5", "6", "7"};
    const TrProp *p;
    if (i < 0 || i >= 7) return NULL;
    p = tr_find(numbers[i]);
    return p ? p->english : NULL;
}

int tr_prop_count(void) { return (int)(sizeof(PROPS) / sizeof(PROPS[0])); }

const TrProp *tr_prop_at(int i)
{
    if (i < 0 || i >= tr_prop_count()) return NULL;
    return &PROPS[i];
}

const TrProp *tr_find(const char *number)
{
    int lo = 0, hi = tr_prop_count() - 1;
    if (!number) return NULL;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(PROPS[mid].number, number);
        if (cmp == 0) return &PROPS[mid];
        if (cmp < 0) lo = mid + 1; else hi = mid - 1;
    }
    return NULL;
}

/* Build ancestry from actual numbered remarks. Decimal digits encode logical
 * depth, but missing prefixes are valid and simply skipped. */
int tr_chain_of(const char *number, const TrProp *out[], int max)
{
    char buf[32];
    size_t len, i;
    int count = 0;
    const TrProp *p;
    if (!number || max <= 0) return 0;
    len = strlen(number);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    for (i = 1; i <= len && count < max; i++) {
        memcpy(buf, number, i); buf[i] = '\0';
        if (buf[i - 1] == '.') continue;
        p = tr_find(buf);
        if (p) out[count++] = p;
    }
    p = tr_find(number);
    if (p && (count == 0 || out[count - 1] != p) && count < max) out[count++] = p;
    return count;
}
