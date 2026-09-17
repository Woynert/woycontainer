
# Library.

project('woycontainer', 'c', default_options : ['c_std=gnu11'])
woycontainer_includes = include_directories('./src')
woycontainer_dep = declare_dependency(
  include_directories: woycontainer_includes
)

# Tests.

if not meson.is_subproject()

  warning_flags = [
    '-Wall',
    '-Wextra',
    #'-Wpedantic', # I removed it because I want gnu-statement-expression.
    '-Werror',
    '-Wshadow',
    '-Wno-language-extension-token',

    # Preferences.
    '-Wno-error=unused-parameter',
    '-Wno-error=unused-function',
    '-Wno-error=unused-variable',
    '-Wno-unused-function',
    '-Wno-gnu-statement-expression',
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

  arena_test = executable('arena_test', './src/arena_test.c')
  test('arena_test', arena_test)

  slot_test = executable('slot_test', './src/slot_test.c')
  test('slot_test', slot_test)

  strpool_test = executable('strpool_test', './src/strpool_test.c')
  test('strpool_test', strpool_test)

  array_test = executable('array_test', './src/array_test.c')
  test('array_test', array_test)

  strmap_test = executable('strmap_test', './src/strmap_test.c')
  test('strmap_test', strmap_test)

  umapstr_test = executable('umapstr_test', './src/umapstr_test.c')
  test('umapstr_test', umapstr_test)

  npmap0_test = executable('npmap0_test', './src/hashmaps/npmap0_test.c')
  test('npmap0_test', npmap0_test)

  npmap1_test = executable('npmap1_test', './src/hashmaps/npmap1_test.c')
  test('npmap1_test', npmap1_test)

  #npmap_str0_test = executable('npmap_str0_test', './src/hashmaps/npmap_str0_test.c')
  #test('npmap_str0_test', npmap_str0_test)

  #npmap_str1_test = executable('npmap_str1_test', './src/hashmaps/npmap_str1_test.c')
  #test('npmap_str1_test', npmap_str1_test)

  list_simple_test = executable('list_simple_test', './src/list_simple_test.c')
  test('list_simple_test', list_simple_test)

  npmap_str2_test = executable('npmap_str2_test', './src/hashmaps/npmap_str2_test.c')
  test('npmap_str2_test', npmap_str2_test)

  hm_benchmarks_test = executable('hm_benchmarks_test', './src/hashmaps/hm_benchmarks_test.c')
  test('hm_benchmarks_test', hm_benchmarks_test)

  wmap_test = executable('wmap_test', './src/wmap_test.c')
  test('wmap_test', wmap_test)

endif

