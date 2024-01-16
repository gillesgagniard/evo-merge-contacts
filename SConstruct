import os

vars = Variables()
vars.Add(PathVariable('prefix', 'Evolution install prefix', '/usr/local'))
vars.Add(BoolVariable('debug', 'build in debug mode', False))
env = Environment(variables = vars)
Help(vars.GenerateHelpText(env))

env.ParseConfig('pkg-config evolution-plugin --cflags --libs')
env.Append(CPPFLAGS = ['-Wall', '-Werror'])
if env['debug']:
  env.Append(CPPFLAGS = ['-g'])
else:
  env.Append(CPPFLAGS = ['-O2'])
output_modversion = os.popen('pkg-config evolution-plugin --modversion').read()
evolution_version_2 = '.'.join(output_modversion.rstrip().split('.')[:2])
env['plugin_path'] = os.path.join(env['prefix'], 'lib', 'evolution', evolution_version_2, 'plugins')
env.Append(CPPDEFINES = [('PLUGIN_PATH', '\\"${plugin_path}\\"')])

src = Split('''
merge.c
conflict.c
conflict-dialog.c
''')

lib = env.SharedLibrary('org-gnome-contacts-merge', src)
installed_lib = env.Install('${plugin_path}', lib)

installed_eplug = env.Command('${plugin_path}/org-gnome-contacts-merge.eplug', 'org-gnome-contacts-merge.eplug.orig',  'cat ${SOURCE} | sed -e \'s!@PREFIX@!${plugin_path}!g\' > ${TARGET}')
installed_ui = env.Install('${plugin_path}', 'org-gnome-contacts-merge.ui')

env.Alias('install', (installed_lib, installed_eplug, installed_ui))

