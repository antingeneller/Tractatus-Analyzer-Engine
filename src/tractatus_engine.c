/* tractatus_engine.c -- analyse a natural-language string against the Tractatus.
 *
 * Method (following 6.53): take the string, look for the propositional form
 * "such and such is the case" (4.5). If it is a picture of a possible state
 * of affairs, locate it under 4.2x; if it is a truth-function of such
 * pictures, under 5; if its truth-conditions are degenerate, under 4.46x;
 * if it trades in value, the self, God, or the limits of the world, it is
 * a pseudo-proposition and lands at 6.4x / 5.6x and finally at 7.
 */
#include "tractatus.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ---------- word lists ---------- */
static const char *W_COPULA[]   = {"is","are","was","were","am","be","been","being","isn't","aren't","wasn't",NULL};
static const char *W_CONNECT[]  = {"and","or","but","if","then","therefore","because","unless","implies","iff","however","while",NULL};
static const char *W_NEG[]      = {"not","no","never","nothing","isn't","aren't","wasn't","don't","doesn't","cannot","can't","nor",NULL};
static const char *W_VALUE[]    = {"good","bad","evil","ought","should","must","right","wrong","moral","morally","ethical","ethics",
                                   "virtue","sin","beautiful","beauty","ugly","sublime","noble","duty","shalt","justice","just",
                                   "happiness","happy","unhappy","meaning","purpose","sacred","holy","value","worth","worthy",NULL};
static const char *W_METAPH[]   = {"god","soul","spirit","eternity","eternal","immortal","immortality","death","die","dies","dead",
                                   "mystical","transcendental","being","existence","nothingness","absolute","universe","reality",
                                   "consciousness","self","i","me","my","myself","mind","free","freewill","fate","destiny",NULL};
static const char *W_LOGICTALK[]= {"logic","logical","form","tautology","tautologies","contradiction","proposition","propositions",
                                   "meaning","sense","truth","language","word","words","name","names","refers","reference",
                                   "represents","depicts","picture","symbol","sign","signs",NULL};
static const char *W_WORLDLIMIT[]={"world","everything","all","totality","whole","limit","limits","boundary","beyond","outside",NULL};
static const char *W_MATH[]     = {"plus","minus","equals","times","sum","square","prime","number","numbers","infinity",NULL};
static const char *W_STOP[]     = {"the","a","an","of","to","in","on","at","it","that","this","these","those","there","very",
                                   "so","as","with","by","from","for","its","their","his","her","and","or","but","if","then",
                                   "is","are","was","were","am","be","been","being","not","no","do","does","did","has","have",
                                   "had","will","would","can","could","may","might","shall","should","must","what","who","when",
                                   "where","why","how",NULL};

static int in_list(const char *w, const char **list)
{
    int i;
    for (i = 0; list[i]; i++) if (strcmp(w, list[i]) == 0) return 1;
    return 0;
}

/* ---------- tokenisation ---------- */
typedef struct {
    char tok[TR_MAX_TOKENS][32];
    int  count;
    int  question;
    int  chars;
} TrToks;

static void tokenise(const char *in, TrToks *t)
{
    int i = 0, j;
    memset(t, 0, sizeof(*t));
    t->chars = (int)strlen(in);
    while (in[i] && t->count < TR_MAX_TOKENS) {
        if (in[i] == '?') t->question = 1;
        if (isalpha((unsigned char)in[i]) || in[i] == '\'') {
            j = 0;
            while (in[i] && (isalpha((unsigned char)in[i]) || in[i] == '\'') && j < 31)
                t->tok[t->count][j++] = (char)tolower((unsigned char)in[i++]);
            t->tok[t->count][j] = '\0';
            t->count++;
        } else {
            i++;
        }
    }
}

static int count_in(const TrToks *t, const char **list)
{
    int i, n = 0;
    for (i = 0; i < t->count; i++) if (in_list(t->tok[i], list)) n++;
    return n;
}

static int first_index_in(const TrToks *t, const char **list)
{
    int i;
    for (i = 0; i < t->count; i++) if (in_list(t->tok[i], list)) return i;
    return -1;
}

/* ---------- helpers for the picture decomposition ---------- */
static void collect_names(const TrToks *t, TrAnalysis *a)
{
    int i;
    a->name_count = 0;
    for (i = 0; i < t->count && a->name_count < 8; i++) {
        if (in_list(t->tok[i], W_STOP)) continue;
        if (in_list(t->tok[i], W_CONNECT)) continue;
        snprintf(a->names[a->name_count], sizeof(a->names[0]), "%s", t->tok[i]);
        a->name_count++;
    }
}

static void set_chain(TrAnalysis *a, const char *c0, const char *c1, const char *c2,
                      const char *c3, const char *c4)
{
    const char *src[5]; int i, k = 0;
    src[0]=c0; src[1]=c1; src[2]=c2; src[3]=c3; src[4]=c4;
    memset(a->chain, 0, sizeof(a->chain));
    for (i = 0; i < 5; i++) if (src[i]) a->chain[k++] = src[i];
}

static void set_hits(TrAnalysis *a, const char **list)
{
    int i;
    memset(a->hits, 0, sizeof(a->hits));
    for (i = 0; list[i] && i < TR_MAX_HITS - 1; i++) a->hits[i] = list[i];
}

const char *tr_kind_name(TrKind k)
{
    switch (k) {
    case TR_PICTURE:       return "significant proposition (sinnvoll) -- a picture";
    case TR_COMPOUND:      return "truth-function of elementary propositions";
    case TR_NEGATION:      return "negated picture (same reality, opposite sense)";
    case TR_TAUTOLOGY:     return "tautology -- senseless (sinnlos)";
    case TR_CONTRADICTION: return "contradiction -- senseless (sinnlos)";
    case TR_ETHICAL:       return "value-utterance -- nonsense (unsinnig), shown not said";
    case TR_METAPHYSICAL:  return "pseudo-proposition about the limits of the world";
    case TR_LOGIC_TALK:    return "attempt to say what can only be shown";
    case TR_QUESTION:      return "question, not a proposition";
    case TR_NONSENSE:      return "no determinate sense (unsinnig)";
    }
    return "?";
}

/* ---------- the analyser ---------- */
void tr_analyse(const char *input, TrAnalysis *a)
{
    TrToks t;
    int cop, neg, con, val, met, lgc, wld, mth, i;
    int self_ref = 0;

    memset(a, 0, sizeof(*a));
    tokenise(input ? input : "", &t);

    cop = count_in(&t, W_COPULA);
    neg = count_in(&t, W_NEG);
    con = count_in(&t, W_CONNECT);
    val = count_in(&t, W_VALUE);
    met = count_in(&t, W_METAPH);
    lgc = count_in(&t, W_LOGICTALK);
    wld = count_in(&t, W_WORLDLIMIT);
    mth = count_in(&t, W_MATH);

    for (i = 0; i < t.count; i++)
        if (strcmp(t.tok[i], "sentence") == 0 || strcmp(t.tok[i], "statement") == 0 ||
            strcmp(t.tok[i], "proposition") == 0)
            self_ref = 1;

    collect_names(&t, a);

    /* the relational form asserted, in the style of 4.24 */
    if (a->name_count >= 2)
        snprintf(a->form, sizeof(a->form), "phi(%s, %s)%s", a->names[0], a->names[1],
                 a->name_count > 2 ? " ... (further names present)" : "");
    else if (a->name_count == 1)
        snprintf(a->form, sizeof(a->form), "fx  where x = %s", a->names[0]);
    else
        snprintf(a->form, sizeof(a->form), "(no names isolated -- nothing is named)");

    /* ---- 0. empty ---- */
    if (t.count == 0) {
        static const char *h[] = {"3.14","4.5","7",NULL};
        a->kind = TR_NONSENSE; a->nonsense = 1; a->sayable = 0;
        a->verdict = "Nothing was said. Silence is the one case the book endorses.";
        a->why = "A propositional sign is a fact: its words must be combined in a definite way (3.14). "
                 "No words, no sign, no sense. See 7.";
        set_chain(a, "7", NULL, NULL, NULL, NULL);
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "(nothing)");
        snprintf(a->truth_false, sizeof(a->truth_false), "(nothing)");
        return;
    }

    /* ---- 1. questions ---- */
    if (t.question) {
        static const char *h[] = {"4.003","6.5","6.51","6.52","6.521",NULL};
        a->kind = TR_QUESTION; a->sayable = (val || met || wld) ? 0 : 1;
        if (val || met || (wld && !cop)) {
            a->verdict = "A question that cannot be put: no expressible answer, so no expressible question.";
            a->why = "\"For an answer which cannot be expressed the question too cannot be expressed. "
                     "The riddle does not exist\" (6.5). Such a question is dissolved, not answered (6.521).";
            set_chain(a, "6", "6.5", "6.521", NULL, NULL);
        } else {
            a->verdict = "A genuine question: it fixes a proposition whose truth is to be settled by comparison with reality.";
            a->why = "\"If a question can be put at all, then it can also be answered\" (6.5). "
                     "The corresponding proposition is a picture, and 2.223 tells us to compare it with reality.";
            set_chain(a, "6", "6.5", NULL, NULL, NULL);
        }
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "(a question has no truth-value; its proposition does)");
        snprintf(a->truth_false, sizeof(a->truth_false), "(see 4.024: to understand it is to know what is the case if true)");
        return;
    }

    /* ---- 2. value: ethics and aesthetics ---- */
    if (val && val >= mth) {
        static const char *h[] = {"6.41","6.42","6.421","6.422","6.43","6.522","7",NULL};
        a->kind = TR_ETHICAL; a->nonsense = 1; a->sayable = 0;
        a->verdict = "Not a proposition at all: it tries to say something about value, which lies outside the world.";
        a->why = "\"The sense of the world must lie outside the world... If there is a value which is of value, "
                 "it must lie outside all happening and being-so\" (6.41). Hence \"there can be no ethical propositions\" "
                 "(6.42); ethics is transcendental and one with aesthetics (6.421). What you mean may be entirely right, "
                 "but it shows itself (6.522) and must be passed over in silence (7).";
        set_chain(a, "6", "6.4", "6.41", "6.42", "6.421");
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "no truth-condition: no state of affairs could verify it");
        snprintf(a->truth_false, sizeof(a->truth_false), "no falsity-condition either -- it is not bipolar (2.21)");
        return;
    }

    /* ---- 3. self-reference ---- */
    if (self_ref && cop) {
        static const char *h[] = {"3.332","3.333","4.0031","6.53",NULL};
        a->kind = TR_NONSENSE; a->nonsense = 1; a->sayable = 0;
        a->verdict = "A proposition about propositions: it tries to contain itself, which no sign can do.";
        a->why = "\"No proposition can say anything about itself, because the propositional sign cannot be contained "
                 "in itself\" (3.332), and \"a function cannot be its own argument\" (3.333). The paradoxes vanish "
                 "with the notation that forbids this.";
        set_chain(a, "3", "3.3", "3.332", "3.333", NULL);
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "undefined -- the sign has no legitimate argument place");
        snprintf(a->truth_false, sizeof(a->truth_false), "undefined");
        return;
    }

    /* ---- 4. talk about logic, sense, language, form ---- */
    if (lgc >= 2 || (lgc && (wld || met))) {
        static const char *h[] = {"4.12","4.121","4.1212","5.4","5.43","6.13","6.54",NULL};
        a->kind = TR_LOGIC_TALK; a->nonsense = 1; a->sayable = 0;
        a->verdict = "It attempts to state the logical form or the depicting relation itself -- which can only be shown.";
        a->why = "\"Propositions cannot represent the logical form: this mirrors itself in the propositions... "
                 "That which expresses itself in language, we cannot express by language\" (4.121); "
                 "\"what can be shown cannot be said\" (4.1212). Logic is transcendental (6.13). "
                 "The Tractatus itself is in this class, and says so at 6.54.";
        set_chain(a, "4", "4.1", "4.12", "4.121", "4.1212");
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "none: form is not a fact in the world");
        snprintf(a->truth_false, sizeof(a->truth_false), "none: nothing could count against it");
        return;
    }

    /* ---- 5. the metaphysical subject, the world as a whole, God, death ---- */
    if (met || (wld && (cop == 0 || wld >= 2))) {
        static const char *h[] = {"5.6","5.61","5.62","5.632","5.64","6.431","6.4311","6.432","6.44","6.45","7",NULL};
        a->kind = TR_METAPHYSICAL; a->nonsense = 1; a->sayable = 0;
        a->verdict = "A pseudo-proposition: it speaks of the subject, the world-as-whole, or what lies beyond the world.";
        a->why = "\"The limits of my language mean the limits of my world\" (5.6). "
                 "The subject \"does not belong to the world but is a limit of the world\" (5.632), so it cannot occur "
                 "as a name inside a picture. \"God does not reveal himself in the world\" (6.432), and "
                 "\"not how the world is, is the mystical, but that it is\" (6.44). "
                 "Meaning is felt here, but no fact is pictured (6.522, 7).";
        set_chain(a, "5", "5.6", "5.62", "5.632", "5.64");
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "no comparison with reality is possible (2.223 cannot be applied)");
        snprintf(a->truth_false, sizeof(a->truth_false), "no state of affairs is excluded (4.462)");
        return;
    }

    /* ---- 6. degenerate truth-conditions ---- */
    if (cop && a->name_count >= 2 && strcmp(a->names[0], a->names[1]) == 0) {
        static const char *h[] = {"4.46","4.461","4.4611","5.5303","6.1","6.11",NULL};
        a->kind = TR_TAUTOLOGY; a->senseless = 1; a->sayable = 1;
        a->verdict = "Senseless but not nonsense: its truth-conditions are tautological -- it says nothing.";
        a->why = "The same name stands on both sides, so the sign is true for every truth-possibility of the "
                 "elementary propositions (4.46). \"Tautology and contradiction are without sense\" (4.461) yet "
                 "belong to the symbolism like '0' in arithmetic (4.4611). \"To say of one thing that it is identical "
                 "with itself is to say nothing\" (5.5303).";
        set_chain(a, "4", "4.4", "4.46", "4.461", "4.4611");
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "every possible state of affairs (unconditionally true, 4.464)");
        snprintf(a->truth_false, sizeof(a->truth_false), "none");
        return;
    }
    if (neg && cop && a->name_count >= 2 && strcmp(a->names[0], a->names[1]) == 0) {
        static const char *h[] = {"4.46","4.461","4.462","4.464",NULL};
        a->kind = TR_CONTRADICTION; a->senseless = 1; a->sayable = 1;
        a->verdict = "Senseless: contradictory truth-conditions -- it permits no state of affairs.";
        a->why = "\"In the second case the proposition is false for all the truth-possibilities\" (4.46); "
                 "a contradiction is no picture of reality (4.462) and its truth is impossible (4.464).";
        set_chain(a, "4", "4.4", "4.46", "4.462", NULL);
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "none");
        snprintf(a->truth_false, sizeof(a->truth_false), "every possible state of affairs");
        return;
    }

    /* ---- 7. mathematics ---- */
    if (mth >= 2) {
        static const char *h[] = {"6.2","6.21","6.211","6.22",NULL};
        a->kind = TR_TAUTOLOGY; a->senseless = 1; a->sayable = 1;
        a->verdict = "An equation: a pseudo-proposition of mathematics, a method rather than a thought.";
        a->why = "\"Mathematics is a logical method. The propositions of mathematics are equations, and therefore "
                 "pseudo-propositions\" (6.2). \"Mathematical propositions express no thoughts\" (6.21); we use them "
                 "only to pass between non-mathematical propositions (6.211).";
        set_chain(a, "6", "6.2", "6.21", NULL, NULL);
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "shown in the symbolism itself, not by comparison with reality");
        snprintf(a->truth_false, sizeof(a->truth_false), "none: an equation is not bipolar");
        return;
    }

    /* ---- 8. truth-functional compound ---- */
    if (con && cop) {
        static const char *h[] = {"5","5.01","5.3","4.4","4.431","5.11","5.2341",NULL};
        int ci = first_index_in(&t, W_CONNECT);
        a->kind = TR_COMPOUND; a->sense = 1; a->sayable = 1;
        a->verdict = "A significant proposition, but not an elementary one: a truth-function of elementary propositions.";
        a->why = "The connective joins pictures, so the whole \"is the expression of agreement and disagreement with the "
                 "truth-possibilities of the elementary propositions\" (4.4) and is a result of truth-operations upon them "
                 "(5, 5.3). Its sense is a function of the senses of its parts (5.2341); fully analysed, it reduces to "
                 "elementary propositions (5.01), each of which asserts the existence of an atomic fact (4.21).";
        set_chain(a, "5", "5.01", "5.3", NULL, NULL);
        set_hits(a, h);
        snprintf(a->form, sizeof(a->form), "truth-function joined by \"%s\":  f(p, q)  ->  N(xi-bar) (6)",
                 ci >= 0 ? t.tok[ci] : "and");
        snprintf(a->truth_true, sizeof(a->truth_true),
                 "those rows of the truth-table (4.31) with which the sign agrees");
        snprintf(a->truth_false, sizeof(a->truth_false),
                 "the remaining rows: it is false for those truth-possibilities (4.41)");
        return;
    }

    /* ---- 9. negated elementary picture ---- */
    if (neg && cop) {
        static const char *h[] = {"4.0621","5.2341","4.21","4.022","2.21","2.06",NULL};
        a->kind = TR_NEGATION; a->sense = 1; a->sayable = 1;
        a->verdict = "A significant proposition: the negation of an elementary picture -- same reality, opposite sense.";
        a->why = "\"The propositions 'p' and 'not-p' have opposite senses, but to them corresponds one and the same "
                 "reality\" (4.0621). The negation sign adds no object (5.4); it operates on the sense (5.2341). "
                 "Reality is the existence *and* non-existence of atomic facts (2.06), so a true negation pictures "
                 "an atomic fact's not obtaining.";
        set_chain(a, "4", "4.06", "4.0621", NULL, NULL);
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true),
                 "the atomic fact %s does NOT obtain", a->form);
        snprintf(a->truth_false, sizeof(a->truth_false),
                 "the atomic fact %s obtains", a->form);
        return;
    }

    /* ---- 10. the paradigm case: an elementary picture ---- */
    if (cop && a->name_count >= 2) {
        static const char *h[] = {"2.1","2.12","2.15","2.201","2.21","2.223","4.01","4.021","4.022","4.024","4.21","4.22",NULL};
        a->kind = TR_PICTURE; a->sense = 1; a->sayable = 1;
        a->verdict = "A significant proposition: an elementary proposition asserting the existence of an atomic fact.";
        a->why = "The names are concatenated in a definite way (4.22, 3.14), so the sign is a logical picture: it "
                 "\"presents the existence and non-existence of atomic facts\" (4.1) and \"shows how things stand, "
                 "if it is true; and it says, that they do so stand\" (4.022). You understood it without anyone "
                 "explaining its sense (4.021); to understand it is to know what is the case if it is true (4.024). "
                 "It is bipolar -- it can be false (2.21) -- and only comparison with reality settles which (2.223).";
        set_chain(a, "4", "4.2", "4.21", "4.22", NULL);
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true),
                 "the objects named are really combined as %s asserts (2.15, 2.222)", a->form);
        snprintf(a->truth_false, sizeof(a->truth_false),
                 "those objects are combined otherwise -- the possibility is pictured either way (2.201)");
        return;
    }

    /* ---- 11. no propositional form at all ---- */
    {
        static const char *h[] = {"4.003","4.5","4.4733","5.4733","6.53","7",NULL};
        a->kind = TR_NONSENSE; a->nonsense = 1; a->sayable = 0;
        a->verdict = "Not senseless-but-legitimate: simply nonsense -- some sign here has been given no meaning.";
        a->why = "The general form of a proposition is \"Such and such is the case\" (4.5), and nothing here takes "
                 "that form. \"Every possible proposition is legitimately constructed, and if it has no sense this can "
                 "only be because we have given no meaning to certain of its constituent parts\" (5.4733). "
                 "The right method is to demonstrate exactly that (6.53).";
        set_chain(a, "4", "4.003", NULL, NULL, NULL);
        set_hits(a, h);
        snprintf(a->truth_true, sizeof(a->truth_true), "none -- there is no sense to compare with reality");
        snprintf(a->truth_false, sizeof(a->truth_false), "none");
    }
}
