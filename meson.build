# In this directory, execute 'meson build/' once
# After that: 'ninja test -C build/'
# meson build/ --reconfigure --buildtype=debug
# meson build/ --reconfigure --buildtype=debugoptimized
# meson build/ --reconfigure --buildtype=release

project('inspect', 'cpp',
        default_options: ['cpp_std=c++2a', 'warning_level=3'],
        version: '0.1.0',
        license: 'GPL3')
# Uncomment for profiling.
add_global_arguments('-pg', language: 'cpp')

inspect_sources = ['inspect.cc', 'main.cc']
#inspect_app = executable('inspect', inspect_sources)
inspect_app = executable('inspect', inspect_sources, link_args: '-pg')

write_sources = ['write.cc']
write_app = executable('write', write_sources)

test_sources = ['inspect.cc', 'test.cc', 'test_inspect.cc']
test_app = executable('test_app', test_sources)
test('inspector test', test_app)
