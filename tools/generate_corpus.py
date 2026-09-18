#!/usr/bin/env python3
from pathlib import Path
import re, html
SRC=Path(__file__).with_name('tractatus.tex')
OUT=Path(__file__).parents[1]/'src'/'tractatus_corpus.inc'
s=SRC.read_text(encoding='utf-8').replace('\r\n','\n')

def extract(kind):
    tag='\\Proposition'+kind+'{'; pos=0; out=[]
    while True:
        at=s.find(tag,pos)
        if at<0: break
        p=at+len(tag); q=s.index('}',p); num=s[p:q]; p=q+1
        while s[p].isspace(): p+=1
        if s[p]!='{': raise ValueError(num)
        depth=1; q=p+1
        while depth:
            if s[q]=='{' and (q==0 or s[q-1]!='\\'): depth+=1
            elif s[q]=='}' and (q==0 or s[q-1]!='\\'): depth-=1
            q+=1
        out.append((num,s[p+1:q-1])); pos=q
    return out

def arg(text, i):
    while i<len(text) and text[i].isspace(): i+=1
    if i>=len(text) or text[i]!='{': return '',i
    d=1; j=i+1
    while d and j<len(text):
        if text[j]=='{' and text[j-1]!='\\': d+=1
        elif text[j]=='}' and text[j-1]!='\\': d-=1
        j+=1
    return text[i+1:j-1],j

zero = {
 'DotOp':' · ', 'BarOp':' | ', 'Implies':' ⊃ ', 'lor':' ∨ ', 'equiv':' ≡ ',
 'exists':'∃', 'Omega':'Ω', 'xi':'ξ', 'eta':'η', 'kappa':'κ', 'mu':'μ', 'nu':'ν',
 'phi':'φ', 'psi':'ψ', 'aleph':'ℵ', 'ldots':'…', 'fourdots':'....', 'fivedots':'.....',
 'times':'×', 'sum':'Σ', 'vdash':'⊢', 'flat':'♭', 'sharp':'♯',
 'IdEst':'I.e.', 'idEst':'i.e.', 'ExempliGratia':'E.g.', 'exempliGratia':'e.g.',
 'dasHeiszt':'d.h.', 'zumBeispiel':'z.B.', 'ZumBeispiel':'Z.B.', 'undAndere':'u.a.',
 'UndSoWeiter':'U.s.w.', 'undSoFort':'u.s.f.', 'False':'F', 'Wahr':'W',
 'DittoInWords':'in words:', 'DittoInWorten':'in Worten:', 'AllowBreak':'',
 'noindent':'', 'centering':'', 'footnotesize':'', 'enlargethispage':'',
 'stretchyspace':'', 'verystretchyspace':'', 'baselineskip':'', 'textwidth':'',
 'hline':'', 'limits':'', 'Strut':'',
 'glqq':'„', 'grqq':'“', 'glq':'‚', 'grq':'‘', 'quotedblbase':'„'
}
one = {'emph','textit','text','mbox','BookTitle','German','Emph','EmphPart','overline','Not','PropERef','PropGRef','DPtypo'}
discard_one = {'enlargethispage','hspace','vspace','baselineskip'}

def clean(t):
    t=re.sub(r'%[^\n]*','',t)
    # TeX-only discretionary line-break markers are not part of the text.
    t=t.replace('\\-', '')
    t=re.sub(r'\\begin\{table\*\}(?:\[[^]]*\])?','',t); t=re.sub(r'\\end\{table\*\}','',t)
    t=re.sub(r'\\begin\{tabular\}(?:\[[^]]*\])?\{[^}]*\}','',t); t=re.sub(r'\\end\{tabular\}','',t)
    t=t.replace('\\\\',' §ROW§ ').replace('&',' | ')
    t=t.replace('``','“').replace("''",'”').replace('---','—').replace('--','–').replace('~',' ')
    out=[]; i=0
    while i<len(t):
        if t[i]=='$': i+=1; continue
        if t[i]=='\\':
            if i+1<len(t) and not t[i+1].isalpha():
                if t[i+1] in ';,': out.append(' ')
                else: out.append(t[i+1])
                i+=2; continue
            m=re.match(r'\\([A-Za-z]+)',t[i:])
            if not m: out.append(t[i+1] if i+1<len(t) else ''); i+=2; continue
            name=m.group(1); i+=len(m.group(0))
            if name=='footnote':
                a,i=arg(t,i); out.append(' [Numbering note: '+clean(a)+']'); continue
            if name in discard_one:
                a,i=arg(t,i); continue
            if name=='phantom':
                a,i=arg(t,i); continue
            if name=='smash':
                while i<len(t) and t[i].isspace(): i+=1
                if i<len(t) and t[i]=='[': i=t.find(']',i)+1
                a,i=arg(t,i); out.append(clean(a)); continue
            if name in ('DPtypo',):
                a,i=arg(t,i); b,i=arg(t,i); out.append(clean(b)); continue
            if name in one:
                a,i=arg(t,i)
                v=clean(a)
                if name=='Not': v='¬'+v
                elif name=='overline': v=v+'̅'
                out.append(v); continue
            if name=='frac' or name=='binom':
                a,i=arg(t,i); b,i=arg(t,i); out.append(clean(a)+('/' if name=='frac' else ' over ')+clean(b)); continue
            if name=='discretionary':
                a,i=arg(t,i); b,i=arg(t,i); c,i=arg(t,i); out.append(clean(c)); continue
            if name=='Illustration':
                # Preserve the diagram reference without gluing it to prose.
                if i<len(t) and t[i]=='[': i=t.find(']',i)+1
                a,i=arg(t,i); b,i=arg(t,i); out.append('\n[Figure: '+clean(b or a)+']\n'); continue
            if name in ('begin','end'):
                a,i=arg(t,i); continue
            if name in ('raisebox','rule'):
                a,i=arg(t,i); continue
            out.append(zero.get(name,'')); continue
        if t[i] in '{}': i+=1; continue
        out.append(t[i]); i+=1
    t=''.join(out)
    t=t.replace(' ,',',').replace(' .','.').replace(' ;',';').replace(' :',':')
    t=re.sub(r'\s+',' ',t).strip()
    t=t.replace('§ROW§', '\n')
    t=t.replace('No.;', 'No. ')
    t=t.replace('”is ', '” is ')
    return html.unescape(t).strip()

def c(s):
    return '"'+s.replace('\\','\\\\').replace('"','\\"').replace('\n','\\n')+'"'
E=extract('E'); G=extract('G')
assert len(E)==len(G)==526
assert [x[0] for x in E]==[x[0] for x in G]
rows=[]
for (n,e),(_,g) in zip(E,G): rows.append('    {%s, %s, %s},' % (c(n),c(clean(e)),c(clean(g))))
header='''/* Generated from Project Gutenberg eBook #5740 (public domain in the US).\n * C. K. Ogden / F. P. Ramsey English translation and Wittgenstein's German.\n * Regenerate with: python3 tools/generate_corpus.py\n */\nstatic const TrProp PROPS[] = {\n'''
OUT.write_text(header+'\n'.join(rows)+'\n};\n',encoding='utf-8')
print(f'wrote {OUT}: {len(rows)} bilingual propositions')
