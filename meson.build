project(
	'wshowkeys',
	'c',
	version: '0.1.0',
	license: 'GPL',
	meson_version: '>=0.48.0',
	default_options: [
		'c_std=c11',
		'warning_level=2',
		'werror=true',
	],
)

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments([
	'-D_POSIX_C_SOURCE=200809L',

	'-Wundef',
	'-Wlogical-op',
	'-Wmissing-include-dirs',
	'-Wold-style-definition',
	'-Wpointer-arith',
	'-Winit-self',
	'-Wstrict-prototypes',
	'-Wimplicit-fallthrough=2',
	'-Wendif-labels',
	'-Wstrict-aliasing=2',
	'-Woverflow',

	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-parameter',
]), language: 'c')

add_project_arguments([
	'-DINPUTDEVPATH="@0@"'.format(get_option('devpath')),
], language: 'c')

cairo          = dependency('cairo')
libinput       = dependency('libinput')
pango          = dependency('pango')
pangocairo     = dependency('pangocairo')
udev           = dependency('libudev')
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols')
xkbcommon      = dependency('xkbcommon')

rt = cc.find_library('rt')

subdir('protocols')

executable(
	'wshowkeys',
	files(
		'devmgr.c',
		'main.c',
		'pango.c',
		'shm.c',
	),
	dependencies: [
		cairo,
		client_protos,
		libinput,
		pango,
		pangocairo,
		rt,
		udev,
		wayland_client,
		wayland_protos,
		xkbcommon,
	],
	install: true,
)
