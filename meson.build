# In this directory, execute 'meson build/' once
# After that: 'ninja test -C build/'
# meson build/ --reconfigure --buildtype=debug
# meson build/ --reconfigure --buildtype=debugoptimized
# meson build/ --reconfigure --buildtype=release

project('inspect', 'cpp',
        default_options: ['cpp_std=c++2a', 'warning_level=3'],
        version: '0.3.0',
        license: 'GPL3')

# Uncomment for profiling. See also src/meson.build
#add_global_arguments('-pg', language: 'cpp')

subdir('src')
subdir('test')
