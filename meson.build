project('jemalloc', 'c', version: '5.2.1')

compiler = meson.get_compiler('c')
link_deps = []
c_flags = ['-DJEMALLOC_NO_PRIVATE_NAMESPACE']

c_flags += [
    compiler.first_supported_argument('-Wno-missing-braces'),
    compiler.first_supported_argument('-Wno-ignored-attributes'),
]

incdirs = ['include']
c_flags += ['-D_REENTRANT']

if target_machine.system() == 'windows'
    incdirs += ['mingw/include']
    c_flags += ['-D_WIN32']
endif

if target_machine.system() == 'linux'
    incdirs += ['linux/include']
    c_flags += ['-D_GNU_SOURCE']
endif

sources = [
    'src/arena.c',
    'src/background_thread.c',
    'src/base.c',
    'src/bin.c',
    'src/bin_info.c',
    'src/bitmap.c',
    'src/buf_writer.c',
    'src/cache_bin.c',
    'src/ckh.c',
    'src/counter.c',
    'src/ctl.c',
    'src/decay.c',
    'src/div.c',
    'src/ecache.c',
    'src/edata.c',
    'src/edata_cache.c',
    'src/ehooks.c',
    'src/emap.c',
    'src/eset.c',
    'src/exp_grow.c',
    'src/extent.c',
    'src/extent_dss.c',
    'src/extent_mmap.c',
    'src/fxp.c',
    'src/hook.c',
    'src/hpa.c',
    'src/hpa_central.c',
    'src/hpdata.c',
    'src/inspect.c',
    'src/jemalloc.c',
    'src/large.c',
    'src/log.c',
    'src/malloc_io.c',
    'src/mutex.c',
    'src/mutex_pool.c',
    'src/nstime.c',
    'src/pa.c',
    'src/pa_extra.c',
    'src/pac.c',
    'src/pages.c',
    'src/pai.c',
    'src/peak_event.c',
    'src/prof.c',
    'src/prof_data.c',
    'src/prof_log.c',
    'src/prof_recent.c',
    'src/prof_stats.c',
    'src/prof_sys.c',
    'src/psset.c',
    'src/rtree.c',
    'src/safety_check.c',
    'src/sc.c',
    'src/sec.c',
    'src/stats.c',
    'src/sz.c',
    'src/tcache.c',
    'src/test_hooks.c',
    'src/thread_event.c',
    'src/ticker.c',
    'src/tsd.c',
    'src/witness.c',
]

if target_machine.system() == 'darwin'
    sources += [
        'src/zone.c',
    ]
endif

incdirs = include_directories(incdirs)

lib = static_library(
    'jemalloc',
    sources,
    include_directories: incdirs,
    dependencies: link_deps,
    c_args: c_flags,
)

jemalloc_dep = declare_dependency(
    include_directories: incdirs,
    dependencies: link_deps,
    link_with: lib,
    version: meson.project_version()
)

meson.override_dependency('jemalloc', jemalloc_dep)

# vim: set ts=4 sts=4 sw=4 et:
