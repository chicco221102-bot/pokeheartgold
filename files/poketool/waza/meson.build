build_dir = meson.current_build_dir()

############################################################
###                       WAZA_TBL                       ###
############################################################
waza_tbl_json = files('waza_tbl.json')
waza_tbl_template = files('waza_tbl.json.txt')

waza_tbl_narc = custom_target('waza_tbl.narc',
    input : waza_tbl_json,
    output: [
        'waza_tbl.narc',
        'waza_tbl.naix'
    ],
    command : [
        json2bin_sh,
        '--compiler', ' '.join(meson.get_compiler('c', native: false).cmd_array()),
        '--flags', ' '.join(c_args),
        '--jsonproc', jsonproc_exe.full_path(),
        '--template', waza_tbl_template[0].full_path(),
        '--o2narc', o2narc_exe.full_path(),
        '--out-dir', build_dir,
        '-i', relative_source_root,
        '-i', relative_source_root / 'include',
        '-i', relative_source_root / 'subprojects' / 'NitroSDK-4.2.30001' / 'include',
        '--language', 'c',
        '-N',
        '-p', '0x00',
        '@INPUT@'
    ],
    depends : [
        jsonproc_exe,
        o2narc_exe,
    ],
)

nitrofs_files += waza_tbl_narc
naix_headers += waza_tbl_narc[1]
