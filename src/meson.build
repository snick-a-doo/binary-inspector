inspect_sources = ['inspect.cc', 'main.cc']
threads = dependency('threads')
inspect_app = executable('inspect',
                         inspect_sources,
                         dependencies: [threads],
                         install: true)
# Uncomment for profiling.
#inspect_app = executable('inspect', inspect_sources, link_args: '-pg')
