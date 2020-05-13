project('mooned', 'c')

gtkdep = dependency('gtk+-3.0')

gnome=import('gnome')
asresources = gnome.compile_resources('asresources','mooned.gresource.xml', c_name: 'as')

sourcefiles=files('main.c', 'moonedapp.c', 'moonedwin.c')

executable('mooned', sourcefiles, asresources, dependencies: gtkdep, install: true)
