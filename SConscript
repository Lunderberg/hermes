# -*- python -*-

import os

Import('env')

env.Append(CPPPATH=['/usr/local/include'])
env.Append(CPPPATH=['#/include'])

env.Append(CPPFLAGS=['-g'])

output = []
for src in Glob('*.cc'):
    exe = env.Program([src,Glob('src/*.cc')])
    output.extend(exe)

Return('output')
