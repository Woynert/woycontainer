
# Library.

project('woycontainer', 'c', default_options : ['c_std=gnu11'])
woycontainer_includes = include_directories('./src')
woycontainer_dep = declare_dependency(
  include_directories: woycontainer_includes
)

# Tests.

warning_flags = [
  '-Wall',
  '-Wextra',
  '-Wpedantic',
  '-Werror',
  '-Wshadow',
  '-Wno-language-extension-token',

  # Preferences.
  '-Wno-error=unused-parameter',
  '-Wno-error=unused-function',
  '-Wno-error=unused-variable',
]
san_flags = [
  '-fsanitize=address,undefined',
  '-fno-omit-frame-pointer',
  '-g',
  '-O1', # O1 supposedly better than O0 or O2 for debug.
]
link_flags = ['-fsanitize=address,undefined']

add_project_arguments(warning_flags + san_flags, language: 'c')
add_project_link_arguments(link_flags, language: 'c')

arena_test = executable('arena_test' ,'./src/arena_test.c')
test('arena_test', arena_test)
