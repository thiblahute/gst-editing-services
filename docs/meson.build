html_theme = 'https://github.com/hotdoc/hotdoc_lumen_theme/releases/download/0.6/hotdoc_lumen_theme-0.6.tar.xz?sha256=0e2f175f4cf8c00ed7ac5014e30c806a294b0d3818565eb74e1424a948e8a452'
hotdoc_p = find_program('hotdoc', required: get_option('doc'))
build_hotdoc = false

hotdoc_extension_dir = []
required_hotdoc_extensions = ['gi-extension']
if gst_dep.type_name() == 'internal'
    gst_proj = subproject('gstreamer')
    plugins_cache_generator = gst_proj.get_variable('plugins_cache_generator')
    if gst_proj.get_variable('build_hotdoc')
        hotdoc_extension_dir = [gst_proj.get_variable('hotdoc_extension_dir')]
    else
        required_hotdoc_extensions += ['gst-extension']
    endif
else
    required_hotdoc_extensions += ['gst-extension']
    plugins_cache_generator = find_program(join_paths(gst_dep.get_pkg_variable('libexecdir'), 'gstreamer-' + apiversion, 'gst-plugins-doc-cache-generator'))
endif

plugins = [nle]
plugins_cache = join_paths(meson.current_source_dir(), 'gst_plugins_cache.json')

if plugins_cache_generator.found()
    plugins_doc_dep = custom_target('editing-services-doc-cache',
        command: [plugins_cache_generator, plugins_cache, '@INPUT@'],
        input: plugins,
        output: 'gst_plugins_cache.json',
    )
else
    warning('GStreamer plugin inspector for documentation not found, can\'t update the cache')
endif

if not hotdoc_p.found()
    message('Hotdoc not found, not building the documentation')
    subdir_done()
endif

hotdoc = import('hotdoc')
if not hotdoc.has_extensions(required_hotdoc_extensions)
    if get_option('doc').enabled()
        error('Documentation enabled but gi-extension missing')
    endif

    message('@0@ extensions not found, not building documentation'.format(required_hotdoc_extensions))
    subdir_done()
endif

if not build_gir
    if get_option('doc').enabled()
        error('Documentation enabled but introspection not built.')
    endif

    message('Introspection not built, can\'t build the documentation')
    subdir_done()
endif

build_hotdoc = true
ges_excludes = []
foreach f: ['gesmarshal.*',
            'ges-internal.*',
            'ges-auto-transition.*',
            'ges-structured-interface.*',
            'ges-structure-parser.*',
            'ges-version.h',
            'ges-smart-*',
            'ges-command-line-formatter.*',
            'ges-base-xml-formatter.h',
            'gstframepositioner.*',
            'lex.priv_ges_parse_yy.c',
            'ges-parse-lex.[c]']
    ges_excludes += [join_paths(meson.current_source_dir(), '..', '..', 'ges', f)]
endforeach

hotdoc = import('hotdoc')
libs_doc = [hotdoc.generate_doc('gst-editing-services',
    project_version: apiversion,
    extra_assets: [join_paths(meson.current_source_dir(), 'images')],
    gi_c_sources: ges_sources + ges_headers,
    gi_c_source_roots: [join_paths(meson.current_source_dir(), '../ges/')],
    gi_sources: [ges_gir[0].full_path()],
    gi_c_source_filters: ges_excludes,
    sitemap: 'sitemap.txt',
    index: 'index.md',
    gi_index: 'index.md',
    gi_smart_index: true,
    gi_order_generated_subpages: true,
    dependencies: [ges_dep],
    disable_incremental_build: true,
    extra_extensions_paths: hotdoc_extension_dir,
)]

plugins_doc = [hotdoc.generate_doc('nle',
    project_version: apiversion,
    extra_assets: [join_paths(meson.current_source_dir(), 'images')],
    sitemap: 'nle-sitemap.txt',
    index: 'nle-index.md',
    dependencies: [nle],
    disable_incremental_build: true,
    extra_extensions_paths: hotdoc_extension_dir,
    gst_order_generated_subpages: true,
    gst_index: 'nle-index.md',
    gst_c_sources: ['../../plugins/nle/*.[ch]'],
    gst_smart_index: true,
    gst_cache_file: plugins_cache,
    gst_plugin_name: 'nle',
)]