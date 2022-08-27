set(app_src sl_btmesh_ncp_host.c sl_bt_ncp_host_api.c sl_bt_ncp_host.c 
   sl_btmesh.c sl_btmesh_ncp_host.c sl_btmesh_ncp_host_api.c sl_status.c sl_string.c btmesh_conf.c btmesh_conf_job.c btmesh_conf_task.c
   btmesh_prov.c btmesh_db.c system.c app_log/app_log.c btmesh_conf_distributor.c btmesh_conf_executor.c iostream_mock/sl_iostream_handles.c simple_timer/sl_simple_timer
   src/sl_slist.c emlib/src/em_assert.c sl_btmesh_lib.c memory_manager/sl_malloc.c sl_btmesh_serdeser.c)

set(req app_uart)
set(dir "." "config" "inc" "protocol/bluetooth/inc" "app_log" "app_log/config" "iostream_mock" "simple_timer" "src" "emlib/inc" "app_btmesh_util" "app_btmesh_util/config"
   "memory_manager" "memory_manager/config")
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS ${dir}
                    REQUIRES ${req})

