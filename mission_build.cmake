###########################################################
#
# TO_LAB mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the TO_LAB configuration
set(TO_LAB_MISSION_CONFIG_FILE_LIST
  to_lab_fcncode_values.h
  to_lab_interface_cfg_values.h
  to_lab_mission_cfg.h
  to_lab_perfids.h
  to_lab_msg.h
  to_lab_msgdefs.h
  to_lab_msgstruct.h
  to_lab_tbl.h
  to_lab_tbldefs.h
  to_lab_tblstruct.h
  to_lab_topicid_values.h
)

generate_configfile_set(${TO_LAB_MISSION_CONFIG_FILE_LIST})


# The same test script can be utilized on all targets that run this app
add_cfe_app_test(${to_lab_MISSION_DIR}/tests/to_lab_test_methods.py)

