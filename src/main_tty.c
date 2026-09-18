/* main_tty.c -- Arch Linux / POSIX TTY interface.
 *
 * Build:  see Makefile (make)
 * Usage:  ./tractatus-tty                 interactive
 *         ./tractatus-tty "the sky is red"  one-shot
 *         ./tractatus-tty --no-color, --tree, --prop 4.021
 */
#include "tractatus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int use_color = 1;
static TrLanguage language = TR_ENGLISH;

#define C(seq) (use_color ? seq : "")
#define DIM    C("\033[2m")
#define B      C("\033[1m")
#define R      C("\033[0m")
#define CY     C("\033[38;2;131;165;152m")
#define YE     C("\033[38;2;250;189;47m")
#define GR     C("\033[38;2;184;187;38m")
#define MA     C("\033[38;2;211;134;155m")
#define RD     C("\033[38;2;251;73;52m")

static void rule(void) { printf("%s%s%s\n", DIM,
    "--------------------------------------------------------------------------", R); }

static void wrap(const char *s, int width, const char *indent)
{
    int col = 0;
    const char *p = s;
    printf("%s", indent);
    while (*p) {
        const char *w = p;
        int len = 0;
        if (*p == '\n') { printf("\n%s", indent); p++; col = 0; continue; }
        while (*p && *p != ' ' && *p != '\n') { p++; len++; }
        while (*p == ' ') p++;
        if (col + len > width) { printf("\n%s", indent); col = 0; }
        printf("%.*s ", len, w);
        col += len + 1;
    }
    printf("\n");
}

static void wrap_continuation(const char *s, int width, const char *indent)
{
    int col = 0;
    const char *p = s;
    while (*p) {
        const char *w = p; int len = 0;
        if (*p == '\n') { printf("\n%s", indent); p++; col = 0; continue; }
        while (*p && *p != ' ' && *p != '\n') { p++; len++; }
        while (*p == ' ') p++;
        if (col + len > width) { printf("\n%s", indent); col = 0; }
        printf("%.*s ", len, w); col += len + 1;
    }
    printf("\n");
}

static void print_prop(const char *number, int depth, int highlight)
{
    const TrProp *p = tr_find(number);
    char ind[64];
    int i, n = depth * 2;
    if (n > 40) n = 40;
    for (i = 0; i < n; i++) ind[i] = ' ';
    ind[n] = '\0';
    if (!p) { printf("%s%s%s%s  (structural node, no remark in the text)%s\n", ind, DIM, number, "", R); return; }
    printf("%s%s%s%-8s%s %s\n", ind, highlight ? B : "", highlight ? YE : CY, p->number, R, "");
    wrap(tr_text(p, language), 70 - n, ind);
}

static void show_tree(void)
{
    int i, n = tr_prop_count();
    printf("\n%sTractatus Logico-Philosophicus Engine — complete %s text (%d propositions)%s\n",
           B, language == TR_GERMAN ? "German" : "Ogden/Ramsey", n, R);
    rule();
    for (i = 0; i < n; i++) {
        const TrProp *p = tr_prop_at(i);
        int depth = 0;
        const char *d = strchr(p->number, '.');
        if (d) depth = (int)strlen(d + 1);
        if (depth == 0 && i) rule();
        char indent[64]; int pad = depth ? (depth - 1) * 3 + 12 : 11;
        int j; if (pad > 60) pad = 60;
        for (j = 0; j < pad; j++) indent[j] = ' ';
        indent[pad] = '\0';
        printf("%*s%s%s %-8s%s ", depth ? (depth - 1) * 3 : 0, "",
               CY, depth ? "├─" : "◆", p->number, R);
        wrap_continuation(tr_text(p, language), 68 - depth * 3, indent);
    }
    printf("\n");
}

static void report(const char *input)
{
    TrAnalysis a;
    const TrProp *chain[TR_MAX_CHAIN];
    int i, k;

    tr_analyse(input, &a);

    printf("\n");
    rule();
    printf(" %sINPUT%s  \"%s\"\n", B, R, input);
    rule();

    printf("\n %sCLASSIFICATION%s  %s%s%s\n", B, R, MA, tr_kind_name(a.kind), R);
    printf(" %sSTATUS%s        %s%s%s   sayable: %s%s%s\n", B, R,
           a.sense ? GR : (a.senseless ? YE : RD),
           a.sense ? "sinnvoll (has sense)"
                   : a.senseless ? "sinnlos (senseless, part of the symbolism)"
                                 : "unsinnig (nonsense -- a sign lacks meaning)", R,
           a.sayable ? GR : RD, a.sayable ? "yes" : "no -- it can only be shown", R);

    printf("\n %sRULING%s\n", B, R);
    wrap(a.verdict, 68, "   ");

    printf("\n %sLOCATION IN THE NUMBERING%s   %s(n.mno is a comment on n.mn, which comments on n.m)%s\n", B, R, DIM, R);
    for (i = 0; a.chain[i] && i < TR_MAX_CHAIN; i++) {
        const char *next = a.chain[i + 1];
        printf("%s%s%s%s%s", i ? "" : "   ", B, YE, a.chain[i], R);
        if (next) printf(" %s>%s ", DIM, R);
    }
    printf("\n\n");

    /* full ancestry of the most specific node, with the text of each */
    for (i = 0; a.chain[i] && i < TR_MAX_CHAIN; i++) {
        const char *num = a.chain[i];
        int last = (a.chain[i + 1] == NULL);
        print_prop(num, i, last);
        if (!last) printf("\n");
    }

    printf("\n %sWHY%s\n", B, R);
    wrap(a.why, 68, "   ");

    printf("\n %sPICTURE-THEORETIC DECOMPOSITION%s  %s(2.13, 4.22, 4.24)%s\n", B, R, DIM, R);
    printf("   names (candidate simple signs, 3.202):");
    if (a.name_count == 0) printf(" none");
    for (i = 0; i < a.name_count; i++) printf(" %s\"%s\"%s", CY, a.names[i], R);
    printf("\n   asserted form: %s%s%s\n", CY, a.form, R);
    printf("   true if:\n");  wrap(a.truth_true, 64, "      ");
    printf("   false if:\n");  wrap(a.truth_false, 64, "      ");

    printf("\n %sFURTHER RELEVANT REMARKS%s\n", B, R);
    for (i = 0; a.hits[i] && i < TR_MAX_HITS; i++) {
        const TrProp *p = tr_find(a.hits[i]);
        if (!p) continue;
        printf("   %s%-8s%s %.*s%s\n", CY, p->number, R, 58, tr_text(p, language),
               strlen(tr_text(p, language)) > 58 ? "..." : "");
    }

    /* the ladder */
    k = tr_chain_of(a.chain[0] ? a.chain[0] : "7", chain, TR_MAX_CHAIN);
    (void)k;
    if (!a.sayable) {
        printf("\n %s7%s  %s%s%s\n", B, R, YE, tr_text(tr_find("7"), language), R);
    }
    rule();
    printf("\n");
}

static void banner(void)
{
    int i;
    printf("\n%s  TRACTATUS LOGICO-PHILOSOPHICUS ENGINE%s\n", B, R);
    printf("%s  Ludwig Wittgenstein · 526 propositions · English / Deutsch%s\n\n", DIM, R);
    for (i = 0; i < 7; i++) {
        char number[2] = {(char)('1' + i), '\0'};
        const TrProp *p = tr_find(number);
        if (p) printf("   %s%s  %s%s\n", CY, p->number, tr_text(p, language), R);
    }
    printf("\n%s  Commands: :tree  :prop N  :lang en|de  :seven  :quit%s\n\n", DIM, R);
}

int main(int argc, char **argv)
{
    char line[TR_MAX_INPUT];
    int i, oneshot = 0;
    char buf[TR_MAX_INPUT];

    buf[0] = '\0';
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0) { use_color = 0; continue; }
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            language = strcmp(argv[++i], "de") == 0 ? TR_GERMAN : TR_ENGLISH; continue;
        }
        if (strcmp(argv[i], "--tree") == 0)     { show_tree(); return 0; }
        if (strcmp(argv[i], "--prop") == 0 && i + 1 < argc) {
            print_prop(argv[++i], 0, 1); return 0;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("usage: tractatus-tty [--no-color] [--lang en|de] [--tree] [--prop N] [\"sentence\"]\n");
            return 0;
        }
        strncat(buf, argv[i], sizeof(buf) - strlen(buf) - 2);
        strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
        oneshot = 1;
    }
    if (oneshot) { size_t L = strlen(buf); if (L && buf[L-1] == 0x20) buf[L-1] = 0; report(buf); return 0; }

    banner();
    for (;;) {
        printf("%s%s?%s ", B, GR, R);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;
        if (strcmp(line, ":quit") == 0 || strcmp(line, ":q") == 0) break;
        if (strcmp(line, ":tree") == 0) { show_tree(); continue; }
        if (strcmp(line, ":seven") == 0) {
            const char *nums[] = {"1","2","3","4","5","6","7"};
            for (i = 0; i < 7; i++) printf("   %s%s  %s%s\n", CY, nums[i], tr_text(tr_find(nums[i]), language), R);
            continue;
        }
        if (strcmp(line, ":lang de") == 0) { language = TR_GERMAN; printf("Deutsch\n"); continue; }
        if (strcmp(line, ":lang en") == 0) { language = TR_ENGLISH; printf("English — Ogden/Ramsey\n"); continue; }
        if (strncmp(line, ":prop", 5) == 0) {
            char *p = line + 5;
            while (*p == ' ') p++;
            print_prop(p, 0, 1);
            continue;
        }
        report(line);
    }
    printf("\n%s%s%s\n\n", YE, tr_text(tr_find("7"), language), R);
    return 0;
}
