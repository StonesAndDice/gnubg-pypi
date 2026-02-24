# meson.build
project(
    'gnubg',                      # Your project name
    ['cpp', 'c'],                 # Languages used
    version: run_command(
        ['src/_build_utils/_version.py'],
        check: true
    ).stdout().strip(),
    meson_version : '>=1.7.0',     # Require a recent Meson
)

cc = meson.get_compiler('c')

python3 = import('python').find_installation('python3', pure: false)
python_dep = python3.dependency()
pkgdir = python3.get_install_dir()
pkg_name = 'gnubg'

configure_file(
    input: 'src/gnubgmodule/__init__.py',
    output: '__init__.py',
    copy: true,
    install: false, # We already handle installation via install_data
    install_dir: 'gnubg' # This places it in build/gnubg/__init__.py
)

# Custom target to generate __version__.py at build time and install it
generate_version = custom_target(
    'generate_version',
    output: '_version.py', # Changed from __version__.py
    command: [
        python3,
        files('src/_build_utils/_version.py'),
        '--write', '@OUTPUT@'
    ],
    build_by_default: true,
    install: true,
    install_dir: join_paths(pkgdir, pkg_name)
)

# Install __init__.py
install_data(
    'src/gnubgmodule/__init__.py',
    install_dir: join_paths(pkgdir, pkg_name)
)

install_data(
    'src/gnubgmodule/data/gnubg.weights',
    'src/gnubgmodule/data/gnubg_ts0.bd',
    'src/gnubgmodule/data/gnubg_os0.bd',
    'src/gnubgmodule/data/gnubg_os.db',
    install_dir: join_paths(pkgdir, pkg_name, 'data')
)

# Install examples with the package (e.g. site-packages/gnubg/examples/)
install_subdir(
    'src/gnubgmodule/examples',
    install_dir: join_paths(pkgdir, pkg_name),
)

# --- Critical Fixes ---
# -D_GNU_SOURCE: Exposes POSIX symbols
# -U_FORTIFY_SOURCE: Fixes 'sprintf'/'__builtin_va_arg_pack' errors
# -ffast-math: Performance optimization
# -fgnu89-inline: FIXES the "multiple definition of atoi/vprintf" errors while allowing C11 syntax
# On MinGW: -I win32_stub first so winnt.h gets our stub x86intrin.h/emmintrin.h (avoids broken MMX/AVX chain)
c_args = ['-D_GNU_SOURCE', '-U_FORTIFY_SOURCE', '-ffast-math', '-fgnu89-inline']
if host_machine.system() == 'windows'
  win32_stub = join_paths(meson.project_source_root(), 'win32_stub')
  c_args += ['-mno-mmx', '-mno-avx', '-mno-avx2', '-I' + win32_stub]
endif
add_project_arguments(c_args, language: 'c')

# --- Dependencies ---
glib_dep = dependency('glib-2.0', version: '>= 2.22.0')
gobject_dep = dependency('gobject-2.0')
gthread_dep = dependency('gthread-2.0')
m_dep = cc.find_library('m', required: true)
sqlite_dep = dependency('sqlite3', required: false)
png_dep = dependency('libpng', version: '>= 1.6', required: false)
readline_dep = cc.find_library('readline', required: true)
gmp_dep = cc.find_library('gmp', required: true)
winmm_dep = cc.find_library('winmm', required: host_machine.system() == 'windows')

# --- Configuration Updates ---
conf_data = configuration_data()
conf_data.set('USE_PYTHON', 1)
conf_data.set('USE_MULTITHREAD', 1)
conf_data.set('MAX_NUMTHREADS', 48)
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set('HAVE_LIBGMP', 1)
conf_data.set('HAVE_LIB_READLINE', 1)

# Package & Paths
conf_data.set_quoted('PACKAGE', 'gnubg')
conf_data.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
conf_data.set_quoted('AC_DATADIR', join_paths(get_option('prefix'), get_option('datadir')))
conf_data.set_quoted('AC_PKGDATADIR', join_paths(get_option('prefix'), get_option('datadir'), 'gnubg'))
conf_data.set_quoted('AC_DOCDIR', join_paths(get_option('prefix'), get_option('datadir'), 'doc', 'gnubg'))

# Header Checks
check_headers = [
    'fcntl.h', 'unistd.h', 'sys/types.h', 'sys/stat.h', 'sys/time.h',
    'stdlib.h', 'string.h', 'memory.h', 'strings.h', 'inttypes.h', 'stdint.h',
    'sys/resource.h', 'sys/socket.h'
]

foreach h : check_headers
    if cc.has_header(h)
        conf_data.set('HAVE_' + h.underscorify().to_upper(), 1)
    endif
endforeach

# --- Web Browser Configuration ---
if host_machine.system() == 'windows'
    conf_data.set_quoted('OPEN_URL_PROG', '')
else
    browser = find_program('xdg-open', 'sensible-browser', 'firefox', required: false)
    if browser.found()
        conf_data.set_quoted('OPEN_URL_PROG', browser.full_path())
    else
        conf_data.set_quoted('OPEN_URL_PROG', 'xdg-open')
    endif
endif

config_h = configure_file(output: 'config.h', configuration: conf_data)

# --- Custom Target for Credits Generation ---
# This runs credits.sh, which generates credits.c, credits.h, and AUTHORS
credits_gen = custom_target(
    'credits_gen',
    output: ['credits.c', 'credits.h', 'AUTHORS'],
    command: ['sh', files('src/gnubg/credits.sh')],
    capture: false, # The script writes files directly, don't capture stdout
    install: false
)

# --- Include Directories ---
gnubg_inc = include_directories(
    'src/gnubg',
    'src/gnubg/lib',
    'src/gnubgmodule',
)
# On Windows, inject stub mmintrin.h first so MinGW's broken one is not used
libgnubg_inc = gnubg_inc

# --- Source Definitions ---
c_sources = files(
    'src/gnubgmodule/gnubg_lib.c',
    'src/gnubgmodule/python_stubs.c',
    'src/gnubg/non-src/copying.c',
    'src/gnubg/analysis.c',
    'src/gnubg/bearoff.c',
    'src/gnubg/bearoffgammon.c',
    'src/gnubg/boardpos.c',
    'src/gnubg/lib/cache.c',
    'src/gnubg/dbprovider.c',
    'src/gnubg/dice.c',
    'src/gnubg/drawboard.c',  # Needed for ParseMove and FormatMove functions
    'src/gnubg/eval.c',
    'src/gnubg/evallock.c',
    'src/gnubg/export.c',
    'src/gnubg/external.c',
    'src/gnubg/file.c',
    'src/gnubg/format.c',
    'src/gnubg/formatgs.c',
    'src/gnubg/glib-ext.c',
    'src/gnubg/html.c',
    'src/gnubg/htmlimages.c',
    'src/gnubg/import.c',
    'src/gnubg/latex.c',
    'src/gnubg/lib/list.c',
    'src/gnubg/matchequity.c',
    'src/gnubg/matchid.c',
    'src/gnubg/mec.c',
    'src/gnubg/mtsupport.c',
    'src/gnubg/multithread.c',
    'src/gnubg/openurl.c',
    'src/gnubg/osr.c',
    'src/gnubg/output.c',
    'src/gnubg/play.c',
    'src/gnubg/positionid.c',
    'src/gnubg/progress.c',
    'src/gnubg/randomorg.c',
    'src/gnubg/relational.c',
    'src/gnubg/render.c',
    'src/gnubg/renderprefs.c',
    'src/gnubg/rollout.c',
    'src/gnubg/set.c',
    'src/gnubg/sgf.c',
    'src/gnubg/show.c',
    'src/gnubg/simpleboard.c',
    'src/gnubg/sound.c',
    'src/gnubg/speed.c',
    'src/gnubg/text.c',
    'src/gnubg/timer.c',
    'src/gnubg/util.c',
    'src/gnubg/lib/neuralnet.c',
    'src/gnubg/lib/SFMT.c',
    'src/gnubg/lib/isaac.c',
    'src/gnubg/lib/md5.c',
    'src/gnubg/lib/inputs.c',
)

# --- Build Engine ---
libgnubg = static_library(
    'gnubg',
    [c_sources, credits_gen], # Include the generated credits files here
    include_directories: libgnubg_inc,
    dependencies: [glib_dep, gobject_dep, python_dep, m_dep, sqlite_dep, readline_dep, gmp_dep],
    install: false,
)

gnubg_engine_dep = declare_dependency(link_with : libgnubg).as_link_whole()

# --- makeweights: converts gnubg.weights (text) to gnubg.wd (binary) for faster load ---
# This automatically finds the correct libs/paths for the current Python ABI
python_embed_dep = python3.dependency(embed: true)

makeweights_deps = [
    glib_dep, 
    gobject_dep, 
    python_embed_dep, 
    m_dep, 
    sqlite_dep, 
    readline_dep, 
    gmp_dep
]

if host_machine.system() == 'windows'
  makeweights_deps += winmm_dep
endif

makeweights_exe = executable(
    'makeweights',
    'src/gnubg/makeweights.c',
    link_whole : libgnubg,
    include_directories : libgnubg_inc,
    dependencies : makeweights_deps,
    install : false,
)
gnubg_wd = custom_target(
    'gnubg.wd',
    output : 'gnubg.wd',
    input : 'src/gnubgmodule/data/gnubg.weights',
    command : [makeweights_exe, '-f', '@OUTPUT@', '@INPUT@'],
    build_by_default : true,
    install : true,
    install_dir : join_paths(pkgdir, pkg_name, 'data'),
    install_tag : 'python-runtime',
)

# --- Data dir: required for build; installed next to the extension so the package is self-contained ---
fs = import('fs')
data_weights = join_paths(meson.project_source_root(), 'src', 'gnubgmodule', 'data', 'gnubg.weights')
if not fs.exists(data_weights)
  error('Data dir is required: gnubg.weights not found at src/gnubgmodule/data/gnubg.weights. The package loads data from the directory next to the compiled extension (no environment variable needed).')
endif
# Install the full data tree into site-packages/gnubg/data (weights, bearoff, met, etc.)
install_subdir(
    'src/gnubgmodule/data',
    install_dir: join_paths(python3.get_install_dir(), 'gnubg')
)

# --- Build Extension ---
# C++11 required for lambdas, range-for, and nested template '>>' in gnubgmodule.cpp
cpp = meson.get_compiler('cpp')
if cpp.get_id() == 'msvc'
  py_ext_cpp_args = ['/std:c++14']
else
  py_ext_cpp_args = ['-std=c++11', '-fvisibility=default']
endif
# On MinGW, -I win32_stub first so winnt.h gets our stub x86intrin.h/emmintrin.h
if host_machine.system() == 'windows'
  py_ext_cpp_args += ['-mno-mmx', '-mno-avx', '-mno-avx2', '-I' + win32_stub]
endif
py_ext_deps = [python_dep, glib_dep]
if host_machine.system() == 'windows'
  py_ext_deps += [winmm_dep]
else
  py_ext_deps += [cc.find_library('dl', required : true)]
endif
# Ensure gnubg.wd is built (and thus installed) whenever the extension is built (e.g. pip install)
gnubg_wd_dep = declare_dependency(sources : gnubg_wd)
py_ext = python3.extension_module(
    '_gnubg',
    'src/gnubgmodule/gnubgmodule.cpp',
    dependencies : [py_ext_deps, gnubg_wd_dep],
    link_with : libgnubg,
    link_whole : libgnubg,
    include_directories : libgnubg_inc,
    install      : true,
    subdir       : 'gnubg',
    cpp_args     : py_ext_cpp_args,
)

test_env = environment()
test_env.set('PYTHONPATH', [meson.project_build_root()])

test('python_tests',
    python3,
    args: [
        '-m', 'pytest',
        join_paths(meson.project_source_root(), 'tests'),
        join_paths(meson.project_source_root(), 'src/gnubgmodule/examples/rest-api/test_rest_api.py'),
    ],
    env: test_env,
    workdir: meson.project_source_root()
)