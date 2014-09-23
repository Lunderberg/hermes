# -*- python -*-

import os

Import('env')

env.Append(CPPPATH=['/usr/local/include'])
env.Append(CPPPATH=['include'])

libhermes = SConscript('hermes/SConscript',exports=['env'])

env.Append(CPPPATH=['hermes/include'])

output = []
for src in Glob('*.cc'):
    exe = env.Program([src,Glob('src/*.cc'),libhermes])
    output.extend(exe)

Return('output')
