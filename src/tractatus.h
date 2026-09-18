/* tractatus.h -- shared core for the Tractatus analysis engine.
 *
 * The engine holds the structural skeleton of Wittgenstein's
 * Tractatus Logico-Philosophicus (1921): the numbered propositions,
 * their nesting, and the doctrines each one carries. Natural-language
 * input is analysed against those doctrines and located in the tree.
 */
#ifndef TRACTATUS_H
#define TRACTATUS_H

#include <stddef.h>

#define TR_MAX_CHAIN   8
#define TR_MAX_HITS   12
#define TR_MAX_TOKENS 128
#define TR_MAX_INPUT  1024

/* ---- the proposition tree ---- */
typedef struct {
    const char *number;   /* e.g. "4.021" */
    const char *english;  /* Ogden/Ramsey translation */
    const char *german;   /* Wittgenstein's original */
} TrProp;

typedef enum {
    TR_ENGLISH = 0,
    TR_GERMAN = 1
} TrLanguage;

/* ---- doctrinal categories the engine can assign ---- */
typedef enum {
    TR_PICTURE,          /* a genuine bipolar picture of a possible state of affairs */
    TR_COMPOUND,         /* truth-function of elementary propositions */
    TR_NEGATION,         /* sense reversed, same picture */
    TR_TAUTOLOGY,        /* senseless, says nothing, true a priori */
    TR_CONTRADICTION,    /* senseless, excludes every state of affairs */
    TR_ETHICAL,          /* value: shown, not said */
    TR_METAPHYSICAL,     /* pseudo-proposition about world/self/limits */
    TR_LOGIC_TALK,       /* attempts to say what logic can only show */
    TR_QUESTION,         /* not a proposition; possibly not even a question */
    TR_NONSENSE          /* no determinate sense: signs without meaning */
} TrKind;

typedef struct {
    TrKind kind;
    const char *kind_name;
    const char *verdict;        /* one-line ruling */
    const char *why;            /* reasoning in Tractarian terms */
    const char *chain[TR_MAX_CHAIN]; /* primary location, root first, NULL-term */
    const char *hits[TR_MAX_HITS];   /* further relevant proposition numbers */
    int sense;                  /* 1 = has sense (sinnvoll) */
    int senseless;              /* 1 = sinnlos (tautology/contradiction) */
    int nonsense;               /* 1 = unsinnig (pseudo-proposition) */
    int sayable;                /* 1 = can be said; 0 = can only be shown */
    /* picture-theory decomposition */
    char names[8][32];          /* candidate names / objects */
    int  name_count;
    char form[192];              /* the relational form asserted */
    char truth_true[256];       /* what makes it true */
    char truth_false[256];      /* what makes it false */
} TrAnalysis;

/* ---- API ---- */
const TrProp *tr_find(const char *number);              /* exact lookup or NULL */
const char   *tr_text(const TrProp *prop, TrLanguage language);
int           tr_prop_count(void);
const TrProp *tr_prop_at(int i);
int           tr_chain_of(const char *number, const TrProp *out[], int max);
void          tr_analyse(const char *input, TrAnalysis *a);
const char   *tr_kind_name(TrKind k);
const char   *tr_seven(int i);                          /* the 7 cardinal propositions */

#endif /* TRACTATUS_H */
