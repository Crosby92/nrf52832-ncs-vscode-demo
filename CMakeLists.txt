#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_lbs)

zephyr_include_directories(inc)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/gatt_svc.c
  src/bt_conn_ctrl.c
  src/app_threads.c

  # head file
  inc/main.h
  inc/gatt_svc.h
  inc/bt_conn_ctrl.h
  inc/app_threads.h
)

# NORDIC SDK APP END
