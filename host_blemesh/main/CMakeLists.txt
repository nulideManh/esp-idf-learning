
set(app_src app_main.c ncp_host_nothread.c app.c app_conf.c)
set(req )  

idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${req}
                       )
