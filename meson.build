# In this directory, execute 'meson builddir' once
# After that: 'cd builddir; ninja test'

project('inspect', 'cpp',
        default_options: ['cpp_std=c++2a', 'warning_level=3'],
        version: '0.1.0',
        license: 'GPL3')
add_global_arguments('-Dwarning_level=3', language: 'cpp')

inspect_sources = ['inspect.cc', 'main.cc']
inspect_app = executable('inspect', inspect_sources)

write_sources = ['write.cc']
write_app = executable('write', write_sources)

test_sources = ['inspect.cc', 'test.cc', 'test_inspect.cc']
test_app = executable('test_app', test_sources)
test('inspector test', test_app)
