foreach t : ['srgb', 'u_format_test', 'u_format_compatible_test']
  test(t,
    executable(
      t,
      '@0@.c'.format(t),
      include_directories : [inc_include, inc_src, inc_mapi, inc_mesa, inc_gallium, inc_gallium_aux],
      dependencies : idep_mesautil,
    ),
    suite : 'format',
    should_fail : meson.get_cross_property('xfail', '').contains(t),
  )
endforeach
