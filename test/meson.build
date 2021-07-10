write_sources = ['write.cc']
write_app = executable('write', write_sources)

test_sources = ['../src/inspect.cc', 'test.cc', 'test_inspect.cc']
test_app = executable('test_app', test_sources, dependencies: [threads])
test('inspector test', test_app)
