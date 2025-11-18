file(GLOB TASK_SRCS "Task/*.c")

idf_component_register(SRCS
                ${TASK_SRCS}
                "main.c" "esp32_s3_hpy.c"
                INCLUDE_DIRS "." "task"
                )
