#!/usr/bin/env python
#coding: utf-8

import subprocess,re

lang="us1"
tempo=120

translations={
    'us': {
        'battery': 'battery',
        'empty': 'empty',
        'full': 'fully charged',
        'ready': 'ready to work'
        },
    'en': {
        'battery': 'battery',
        'empty': 'empty',
        'full': 'fully charged',
        'ready': 'ready to work'
        },
    'pl': {
        'battery': 'bateria',
        'empty': 'pusta',
        'full': 'naładowana',
        'ready': 'gotowy do pracy'
    },
    'es': {
        'battery': 'batería',
        'empty': 'descargada',
        'full': 'cargada',
        'ready': 'listo para trabajár'
    },
}


words = translations[lang[:2]]

def sentence(text, separate=True, count=0):
    ls=['espeak','-vmb-'+lang, '-s%d' % tempo,'--pho']
    if (separate):
        ls.append('-g1')
    ls.append(text)
    p=subprocess.Popen(ls, stdout = subprocess.PIPE)
    rt=p.communicate()[0]
    rt = rt.decode('utf-8')
    if not separate:
        return rt
    st=rt.split('\n')
    sen=[]
    line=[]
    for a in st:
        a=a.strip()
        if not a:
            continue
        if a.startswith('_'):
            if line:
                sen.append('\n'.join(line))
            line = []
        else:
            line.append(a)
    if line:
        sen.append('\n'.join(line))
    if count:
        while len(sen) > count:
            sen[len(sen)-2] = sen[len(sen)-2]+'\n_ 2\n'+sen[len(sen)-1]
            sen.pop(len(sen)-1)
    return sen

s = sentence("%s %s" % (words['battery'], words['empty']), count=2)
bat = s[0]
levels = [s[1]]
perc = None
for a in range(10,100,10):
    s=sentence("%s %d %%" % (words['battery'],a))
    if not perc:
        perc = s[2]
    levels.append(s[1])
s = sentence("%s %s" % (words['battery'], words['full']), count=2)
levels.append(s[1])
rdy = sentence(words['ready'], False)

out = 'const char *iamready=R"(\n%s\n_ 2\n)";\n' % rdy
out+='const char *battery=R"(\n%s\n_ 2\n)";\n' % bat
out += 'const char *percent=R"(\n%s\n_ 2\n)";\n' % perc
out += 'const char *blevels[]={\n'
for i, a in enumerate(levels):
    if i:
        out += ',\n'
    out += 'R"(\n%s\n)"' % a
out += '};\n'
open('lang_%s.h' % lang[:2],'w').write(out)
        
