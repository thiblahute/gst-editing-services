nle_sources = ['nleobject.c',
    'nlecomposition.c',
    'nleghostpad.c',
    'nleoperation.c',
    'nlesource.c',
    'nleurisource.c'
]

adder = library('gstnle', nle_sources,
  dependencies : [gst_dep, gstbase_dep],
  install : true,
  install_dir : plugins_install_dir,
)

