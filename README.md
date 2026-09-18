# Tractatus Logico-Philosophicus Engine (C)



The following was an attempt to construct (vibe-code) a model which term-indexes a natural language string in relation to the propositions laid out in Wittgenstein's Tractatus Logico-Philosophicus and thereby classify it within early-Wittgenstein's logical atomist model.



# How to use it



Enter a natural language string (preferably a sentence) into the text window and press 'Analyze', the model will attempt to highlight where this sentence stands in relation to the Tractatus. There is also a 'Whole book' section which features the entirety of the Tractatus's 526 numbered propositions. You can also switch between the original German and the Ogden/Ramsey translation (1922, public domain). B)



There are various text-elements that remain rather sketch, as regards the '#LOCATION IN THE NUMBERING' set, root nodes will be marked by a diamond shape, whereas end-nodes remain unmarked. It remains unclear if the model takes a specific stand as to which node a sentence 'belongs to' precisely, but it always relates to the tree somehow, except in mentions of 'God', where the connection between the '#LOCATION IN THE NUMBERING' and the '#WHY?' start to feel rather strange. This unnerves me.



## How to build on Windows



Just launch the .exe, your antivirus might bug you, I still don't know what that's about.



## How to build on Linux



sh
make              # tractatus-tty  (Arch Linux / any POSIX + cc)
make gui          # tractatus.exe  (needs mingw-w64-gcc; also builds natively on Windows)
make both
sudo make install





## TTY usage



sh
./tractatus-tty                    # interactive prompt
./tractatus-tty "god is dead"      # one-shot
./tractatus-tty --tree             # print the whole held structure
./tractatus-tty --prop 4.0621      # print one proposition
./tractatus-tty --no-color



Interactive commands: `:tree`, `:prop N`, `:seven`, `:quit`.



## Possible Answers



|Input|Verdict|
|-|-|
|`the sky is red`|elementary proposition, a picture of an atomic fact — 4.22|
|`the sky is red and the grass is green`|truth-function of elementary propositions — 5.3|
|`the sky is not red`|opposite sense, same reality — 4.0621|
|`the sky is the sky`|tautology, says nothing — 4.4611|
|`torture is wrong`|ethics is transcendental, cannot be said — 6.421|
|`God exists`|no worldly fact is pictured — 6.432 #<- this is the intended answer, model prone to hallucination in relation to God|
|`I am my world`|the subject is a limit, not a part — 5.64|
|`what is the meaning of life?`|a question that cannot be put — 6.5|



## Contents



* `src/tractatus.h` — shared interface.
* `src/tractatus\_data.c` — the numbered propositions: all seven cardinal
propositions plus the decimal expansions that carry the doctrines (picture
theory 2.1–2.225, sense 4.02x, tautology/contradiction 4.46x, truth-functions
5–5.3, the limits of language 5.6–5.641, ethics and the mystical 6.4–6.522,
and 7). The numbering itself encodes the subset relation: `4.0621` comments on
`4.06`, which comments on `4`.
* `src/tractatus\_engine.c` — the analyser. It looks for the general
propositional form "such and such is the case" (4.5) and classifies the input
as one of: picture, truth-functional compound, negation, tautology,
contradiction, equation, value-utterance, metaphysical pseudo-proposition,
attempt to say what can only be shown, question, or plain nonsense. Each
verdict carries its truth-conditions, the names it isolated (3.202), the
asserted form in the style of 4.24, and the chain of propositions it sits under.
* `src/main\_tty.c` — the TTY frontend.
* `src/main\_gui.c` — the Win32 GUI frontend.

