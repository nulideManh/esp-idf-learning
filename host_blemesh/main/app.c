/***************************************************************************//**
 * @file
 * @brief Empty BTmesh NCP-host Example Project.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

/* standard library headers */
// standard library headers
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// app-related headers
#include "app.h"
#include "app_conf.h"
#include "app_log.h"
#include "ncp_host.h"
// #include "app_log_cli.h"
#include "app_assert.h"
// #include "sl_simple_timer.h"
#include "sl_bt_api.h"

#include "sl_btmesh_generic_model_capi_types.h"
#include "sl_btmesh_api.h"

#include "system.h"
#include "sl_bt_api.h"
#include "sl_btmesh_ncp_host.h"
#include "sl_bt_ncp_host.h"
#include "sl_ncp_evt_filter_common.h"
#include "btmesh_conf.h"
#include "btmesh_conf_job.h"
#include "btmesh_prov.h"
#include "btmesh_db.h"
// #include "sl_btmesh_lighting_client.h"
#include "sl_btmesh_lib.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
const char* TAG = "APP";

// -----------------------------------------------------------------------------
// Macros

// Optstring argument for getopt.
#define OPTSTRING      NCP_HOST_OPTSTRING "h"

// Usage info.
#define USAGE          "\n%s " NCP_HOST_USAGE \
  " [-h]"                                     \
  " [--nodeinfo <UUID>]"                      \
  " [--nodelist]"                             \
  " [--provision <UUID>]"                     \
  " [--remove <UUID>]"                        \
  " [--reset]"                                \
  " [--scan]\n"

// Options info.
#define OPTIONS                                                                 \
  "\nOPTIONS\n"                                                                 \
  "NCP Host-related commands:\n"                                                \
  NCP_HOST_OPTIONS                                                              \
  "    -h  Print this help message.\n"                                          \
  "\nHost Provisioner-related commands:\n"                                      \
  "    -i --nodeinfo   Print DCD information about a node in the network\n"     \
  "                    <UUID>     The unique identifier of the node.\n"         \
  "    -l --nodelist   List all nodes present in the provisioner's device "     \
  "database (DDB)\n"                                                            \
  "    -p --provision  Provision a node\n"                                      \
  "                    <UUID>     The UUID of the node to be provisioned. "     \
  "Can be acquired by --scan.\n"                                                \
  "    -r --remove     Remove the given node from the Mesh network\n"           \
  "                    <UUID>     The UUID of the node to be removed\n"         \
  "    -e --reset      Factory reset the provisioner. Note: This command does " \
  "not remove existing devices from the network.\n"                             \
  "    -s --scan       Scan and list unprovisioned beaconing nodes\n"           \
  "\nUUID shall be a string containing 16 separate octets, e.g. "               \
  "\"00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff\"\n"                       \
  "The separator can be any character, but in case of a whitespace character "  \
  "this example requires quotation marks around the string.\n\n"                \
  "Running the program without a Host Provisioner-related command will "        \
  "trigger UI mode.\n\n"

// This characteristic handle value has to match the value in gatt_db.h of
// NCP empty example running on the connected WSTK.
#define GATTDB_SYSTEM_ID 18

// -----------------------------------------------------------------------------
// Static Function Declarations

/***************************************************************************//**
* Handle --nodeinfo functionality
*
*******************************************************************************/
static void handle_nodeinfo(void);

/***************************************************************************//**
* Handle --nodelist functionality
*
*******************************************************************************/
static void handle_nodelist(void);

/***************************************************************************//**
* Handle --provision functionality
*
*******************************************************************************/
static void handle_provision(void);

/***************************************************************************//**
* Handle --remove functionality
*
*******************************************************************************/
static void handle_remove(void);

/***************************************************************************//**
* Handle --reset functionality
*
*******************************************************************************/
static void handle_reset(void);

/***************************************************************************//**
* Handle --scan functionality
*
*******************************************************************************/
static void handle_scan(void);

static void handle_control_onoff(void);
/***************************************************************************//**
*  Node removal job status callback
*
* @param[in] job The job this function is called from
*******************************************************************************/
static void app_on_remove_node_job_status(const btmesh_conf_job_t *job);
void sl_btmesh_generic_base_on_event(sl_btmesh_msg_t *evt);
static void sl_simple_timer_create(void);
void sl_scan_timer_start(void);
void sl_reset_timer_start(void);
/***************************************************************************//**
* Callback for the timer used during scanning
*
* @param[in] timer Pointer to the timer used
* @param[in] data Data from the timer
*******************************************************************************/
static void app_on_scan_timer(TimerHandle_t xTimer);

/***************************************************************************//**
* Callback for the timer used during provisioner reset
*
* @param[in] timer Pointer to the timer used
* @param[in] data Data from the timer
*******************************************************************************/
static void app_on_reset_timer(TimerHandle_t xTimer);

// -----------------------------------------------------------------------------
// Static Variables

/// The state of the current command
static command_state_t command_state = INIT;
/// The command in use
static command_t command = SCAN;
/// UUID passed as an argument
static uuid_128 command_uuid = 
{
  .data = {0x65 , 0x5c ,0xa2 ,0x35 ,0x44 ,0x9b ,0x48 ,0xe9 ,0x89 ,0x61 ,0xa3 ,0x5e ,0x0a ,0xaa ,0x3d ,0xab},
};

static uuid_128 remove_uuid;
/// Flag stating that the btmeshprov_initialized event has arrived
static bool initialized = false;
/// Number of networks already present on the provisioner at startup
static uint16_t networks_on_startup = 0;
/// Number of known devices in the DDB list
static uint16_t ddb_count = 0;

// -----------------------------------------------------------------------------
// Function definitions
void print_uuid(uuid_128 *uuid)
{
  ESP_LOGI(TAG,"UUID:    %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x" ,
          uuid->data[0],
          uuid->data[1],
          uuid->data[2],
          uuid->data[3],
          uuid->data[4],
          uuid->data[5],
          uuid->data[6],
          uuid->data[7],
          uuid->data[8],
          uuid->data[9],
          uuid->data[10],
          uuid->data[11],
          uuid->data[12],
          uuid->data[13],
          uuid->data[14],
          uuid->data[15]);
}

void print_mac(bd_addr *mac)
{
  ESP_LOGI(TAG,"MAC:     %02x:%02x:%02x:%02x:%02x:%02x" ,
          mac->addr[5],
          mac->addr[4],
          mac->addr[3],
          mac->addr[2],
          mac->addr[1],
          mac->addr[0]);
}

void bt_mesh_send_onoff(bool state)
{
  static uint8_t tranID = 1;
  struct mesh_generic_request req;
  sl_status_t sc;

  req.kind = mesh_generic_request_on_off;
  req.on_off = state;

  // sc = mesh_lib_generic_client_publish(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
  //                                      0,
  //                                      tranID,
  //                                      &req,
  //                                      0, // transition time in ms
  //                                      0,
  //                                      0   // flags
  //                                      );

  sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                  0,
                                  0xFFFF,
                                  0,
                                  tranID,
                                  &req,
                                  1000, // transition time in ms
                                  200,
                                  0   // flags
                                  );
  tranID++;
  if (sc == SL_STATUS_OK) {
    app_log_info("send OK\n");
  } else {
    app_log_error("send FAIL\n");
  }
  command_state = FINISHED;
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;
  int opt;

  // Initialize NCP connection.
  sc = ncp_host_init();
  app_log_info("********VERSION 1.0.0********");

  SL_BTMESH_API_REGISTER();
  vTaskDelay(200/portTICK_PERIOD_MS);
  ESP_LOGI(TAG,"Host Provisioner initialised.");
  ESP_LOGI(TAG,"Resetting NCP...");
  // Reset NCP to ensure it gets into a defined state.
  // Once the chip successfully boots, boot event should be received.
  sl_bt_system_reset(sl_bt_system_boot_mode_normal);
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  sl_simple_timer_create();
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  if (initialized) {
    // ESP_LOGI(TAG, ".");
    switch (command) {
      case SCAN:
        handle_scan();
        break;
      case PROVISION:
        handle_provision();
        break;
      case NODELIST:
        handle_nodelist();
        break;
      case NODEINFO:
        handle_nodeinfo();
        break;
      case REMOVE_NODE:
        handle_remove();
        break;
      case RESET:
        handle_reset();
        break;
      case CONTROL_ONOFF:
        handle_control_onoff();
        break;
      case NONE:
        // UI mode
        // handle_ui();
        break;
      default:
        break;
    }
  } 
}

/**************************************************************************//**
 * Application Deinit.
 *****************************************************************************/
void app_deinit(void)
{
  ncp_host_deinit();
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
    {

      // Filter scanner report events as it would send a message every 5 ms
      // and clog UART while scanning for unprovisioned nodes
      uint8_t user_data[SL_NCP_EVT_FILTER_CMD_ADD_LEN];
      uint32_t command_id = sl_bt_evt_scanner_scan_report_id;

      user_data[0] = SL_NCP_EVT_FILTER_CMD_ADD_ID;
      user_data[1] = (command_id >> 0);
      user_data[2] = (command_id >> 8);
      user_data[3] = (command_id >> 16);
      user_data[4] = (command_id >> 24);
      sc = sl_bt_user_manage_event_filter(SL_NCP_EVT_FILTER_CMD_ADD_LEN,
                                          user_data);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to enable filtering on the target\n",
                 (int)sc);

      // Print boot message.
      ESP_LOGI(TAG,"Bluetooth stack booted: v%d.%d.%d-b%d",
                   evt->data.evt_system_boot.major,
                   evt->data.evt_system_boot.minor,
                   evt->data.evt_system_boot.patch,
                   evt->data.evt_system_boot.build);

      // Initialize Mesh stack in Node operation mode,
      // wait for initialized event
      ESP_LOGI(TAG,"Provisioner init");
      sc = sl_btmesh_prov_init();
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to init provisioner\n",
                 (int)sc);
      } break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      // ESP_LOGI(TAG,"ble id: %x",SL_BT_MSG_ID(evt->header));
      break;
  }
}

void app_ui_on_event(sl_btmesh_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
    // case sl_btmesh_evt_prov_device_provisioned_id: {
    //   app_log_info("Provisioning finished" APP_LOG_NEW_LINE);
    //   sl_btmesh_evt_prov_device_provisioned_t *prov_evt =
    //     &evt->data.evt_prov_device_provisioned;
    //   if (PROVISION == command) {
    //     sl_status_t sc = app_conf_start_node_configuration(APP_NETKEY_IDX,
    //                                                        prov_evt->address);
    //     if (SL_STATUS_OK != sc) {
    //       app_log_error("Failed to start configuration procedure." APP_LOG_NEW_LINE);
    //       command_state = FINISHED;
    //     }
    //   } else {
    //     command_state = FINISHED;
    //   }
    //   break;
    // }
    case sl_btmesh_evt_prov_provisioning_failed_id:
      app_log_error("Provisioning failed" APP_LOG_NEW_LINE);
      command_state = FINISHED;
      break;
    // case sl_btmesh_evt_prov_initialized_id: {
    //   sl_btmesh_evt_prov_initialized_t *initialized_evt;
    //   initialized_evt = (sl_btmesh_evt_prov_initialized_t *)&(evt->data);
    //   networks_on_startup = initialized_evt->networks;
    //   break;
    // }
    // case sl_btmesh_evt_prov_ddb_list_id: {
    //   if (NONE != command) {
    //     sl_btmesh_evt_prov_ddb_list_t *ddb_list_evt;
    //     ddb_list_evt = (sl_btmesh_evt_prov_ddb_list_t *)&(evt->data);
    //     uuid_128 uuid = ddb_list_evt->uuid;
    //     uint16_t address = ddb_list_evt->address;
    //     uint8_t elements = ddb_list_evt->elements;
    //     app_log_info("Address: 0x%04x" APP_LOG_NEW_LINE, address);
    //     app_log_info("Element count: %d" APP_LOG_NEW_LINE, elements);
    //     print_uuid(&uuid);
    //   }
    //   break;
    // }
  }
}
/***************************************************************************//**
 * Bluetooth Mesh stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth Mesh stack.
 *****************************************************************************/
void sl_btmesh_on_event(sl_btmesh_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_btmesh_evt_prov_initialized_id: {
      // Device successfully initialized in provisioner mode
      sl_btmesh_evt_prov_initialized_t *initialized_evt;
      initialized_evt = (sl_btmesh_evt_prov_initialized_t *)&(evt->data);

      ESP_LOGI(TAG, "Network initialized");
      ESP_LOGI(TAG, "Networks: %x", initialized_evt->networks);
      ESP_LOGI(TAG, "Address : %x", initialized_evt->address);
      ESP_LOGI(TAG, "IV Index: %x", initialized_evt->iv_index);
      initialized = true;
      networks_on_startup = initialized_evt->networks;

      sl_status_t sc = sl_btmesh_generic_client_init();
      app_assert_status_f(sc, "Failed to init Generic Client");
      sc = sl_btmesh_generic_client_init_on_off();
      app_assert_status_f(sc, "Failed to init on/off client\n");

      // sc = sl_btmesh_generic_client_init_common();
      // app_assert_status_f(sc, "Failed to common init Generic Client\n");

      if (INIT == command_state) {
        command_state = START;
      }
      break;
    }

    case sl_btmesh_evt_prov_initialization_failed_id: {
      // Device failed to initialize in provisioner mode
      sl_btmesh_evt_prov_initialization_failed_t *initialization_failed_evt;
      initialization_failed_evt = (sl_btmesh_evt_prov_initialization_failed_t *)&(evt->data);
      uint16_t res = initialization_failed_evt->result;
      if (0 != res) {
        ESP_LOGE(TAG, "Failed to initialize provisioning node");
      }
      break;
    }
    case sl_btmesh_evt_prov_ddb_list_id:
      // DDB List event
      if (NODELIST == command) {
        // If nodelist is requested, print the information
        // This event can be fired from elsewhere, no logs needed in that case
        sl_btmesh_evt_prov_ddb_list_t *ddb_list_evt;
        ddb_list_evt = (sl_btmesh_evt_prov_ddb_list_t *)&(evt->data);
        uuid_128 uuid = ddb_list_evt->uuid;
        uint16_t address = ddb_list_evt->address;
        uint8_t elements = ddb_list_evt->elements;

        ESP_LOGI(TAG, "Address: 0x%04x", address);
        ESP_LOGI(TAG, "Element count: %d", elements);
        ESP_LOGI(TAG, "UUID:    %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                     uuid.data[0],
                     uuid.data[1],
                     uuid.data[2],
                     uuid.data[3],
                     uuid.data[4],
                     uuid.data[5],
                     uuid.data[6],
                     uuid.data[7],
                     uuid.data[8],
                     uuid.data[9],
                     uuid.data[10],
                     uuid.data[11],
                     uuid.data[12],
                     uuid.data[13],
                     uuid.data[14],
                     uuid.data[15]);
      }
      break;
    case sl_btmesh_evt_node_reset_id:
      // Node reset successful
      ESP_LOGI(TAG, "Reset system");
      sl_bt_system_reset(0);
      break;
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
  // Let other modules handle their events too
  app_ui_on_event(evt);
  btmesh_prov_on_event(evt);
  btmesh_conf_on_event(evt);
  sl_btmesh_generic_base_on_event(evt);
}

// void sl_btmesh_process_event(sl_btmesh_msg_t *evt)
// {
//   sl_btmesh_generic_base_on_event(evt);
// }

// -----------------------------------------------------------------------------
// App logic functions

void handle_scan(void)
{
  switch (command_state) {
    case START: {
      sl_status_t sc;
      sc = btmesh_prov_start_scanning();
      if (SL_STATUS_OK != sc) {
        ESP_LOGW(TAG, "Failed to start scanning");
      } else {
        ESP_LOGI(TAG, "Scanning started");
      }
      command_state = IN_PROGRESS;
      // Let the provisioner scan unprovisioned nodes
      sl_scan_timer_start();
      break;
    }
    case IN_PROGRESS:
      // Do nothing, timer callback will trigger next state
      break;
    case FINISHED: {
      sl_status_t sc;
      sc = btmesh_prov_stop_scanning();
      if (SL_STATUS_OK != sc) {
        ESP_LOGW(TAG, "Failed to stop scanning");
      } else {
        ESP_LOGI(TAG, "Scanning stopped");
      }
      command_state = INIT;
      // exit(EXIT_SUCCESS);
      command_state =  START;
      // handle_nodeinfo();
      command = CONTROL_ONOFF;
      break;
    }
    default:
      // handle_ui_scan();
      break;
  }
}

void handle_provision(void)
{
  switch (command_state) {
    case START:
      ESP_LOGI(TAG, "Provisioning...");
      bd_addr mac = { 0 };
      uint16_t netkey_index = APP_NETKEY_IDX;
      // Try to create a network with index 0.
      // If one is already present, SL_STATUS_BT_MESH_ALREADY_EXISTS is handled
      sl_status_t sc = btmesh_prov_create_network(netkey_index, 0, 0);
      app_assert((sc == SL_STATUS_OK || sc == SL_STATUS_BT_MESH_ALREADY_EXISTS),
                 "Failed to create network");
      if (SL_STATUS_OK == sc) {
        uint8_t appkey_data[16];
        size_t appkey_length;
        // If a new network is created then application key is created as
        // well because the appkeys are bound to network keys.
        // If the network already exists then it is not necessary to create
        // appkey because it has already been created.
        // Note: Output buffer is mandatory for create appkey API function
        sl_status_t sc = btmesh_prov_create_appkey(netkey_index,
                                                   APP_CONF_APPKEY_INDEX,
                                                   0,
                                                   NULL,
                                                   sizeof(appkey_data),
                                                   &appkey_length,
                                                   &appkey_data[0]);
        app_assert_status_f(sc, "Failed to create appkey");
      }
      // MAC address is unknown here, but the database requires a bd_addr struct
      // so we use a 0 here.
      // Note: this won't affect provisioning as only UUID is used there
      // sc = btmesh_prov_start_provisioning(netkey_index, command_uuid, mac, 0, 0);
      sc = btmesh_prov_start_provisioning_by_uuid(netkey_index, command_uuid, 0);
      if(sc != SL_STATUS_OK)
      {
        ESP_LOGE(TAG, "btmesh_prov_start_provisioning FAIL");
      }
      command_state = IN_PROGRESS;
      break;
    case IN_PROGRESS:
      // Wait for provision to end. The btmesh_prov module informs the
      // application on finish.
      break;
    case FINISHED:
      ESP_LOGI(TAG, "Provisioning finished");
      command_state = INIT;
      
      command_state =  START;
      // handle_nodeinfo();
      command = CONTROL_ONOFF;
      // exit(EXIT_SUCCESS);
      break;
    default:
      // handle_ui_provision();
      break;
  }
}

static void handle_control_onoff(void)
{
  switch (command_state) {
    case START:
    {
      bt_mesh_send_onoff(1);
      command_state = IN_PROGRESS;
    }  break;
    case IN_PROGRESS:
      // Wait for configurator to finish nodeinfo printing
      break;
    case FINISHED:
      command_state = INIT;
      break;
    default:
      break;
  }  
}

void handle_nodelist(void)
{
  switch (command_state) {
    case START:
      // Check if any networks are present on the node on startup
      if (0 < networks_on_startup) {
        app_log_info("Getting node list..." APP_LOG_NEW_LINE);
        sl_status_t sc = btmesh_prov_list_provisioned_nodes();
        if (SL_STATUS_EMPTY == sc) {
          app_log_warning("No nodes present in the network" APP_LOG_NEW_LINE);
        }
      }
      else 
      {
        ESP_LOGI(TAG, "No networks present on the device");
      }
      command_state = FINISHED;
      break;
    case IN_PROGRESS:
      // Wait for all nodes' information
      break;
    case FINISHED:
      command_state = INIT;
      // exit(EXIT_SUCCESS);
      break;
    default:
      // handle_ui_nodelist();
      break;
  }
}

void handle_nodeinfo(void)
{
  switch (command_state) {
    case START:
    {
      // Nodeinfo is started automatically after startup DDB list query  
      // sl_status_t sc = btmesh_prov_list_provisioned_nodes();
      // if (SL_STATUS_EMPTY == sc) {
      //   app_log_warning("No provisioned nodes available" APP_LOG_NEW_LINE);
      //   command_state = FINISHED;
      // }
      // app_conf_print_nodeinfo_by_id(0);
      // app_conf_print_nodeinfo_by_id(1);
      command_state = IN_PROGRESS;
    }  break;
    case IN_PROGRESS:
      // Wait for configurator to finish nodeinfo printing
      break;
    case FINISHED:
      app_log_info("All nodes listed" APP_LOG_NEW_LINE);
      command_state = INIT;
      // exit(EXIT_SUCCESS);
      break;
    default:
      // handle_ui_nodeinfo();
      break;
  }
}

void remove_node_job_status_ui(const btmesh_conf_job_t *job)
{
  if (BTMESH_CONF_JOB_RESULT_SUCCESS == job->result) {
    ESP_LOGI(TAG,"Node removed from network");
    sl_status_t sc = btmesh_prov_delete_ddb_entry(remove_uuid);
    ESP_LOGW(TAG, "Failed to delete DDB entry");
  } else {
    ESP_LOGE(TAG, "Could not remove node from network");
  }
  command_state = UI_FINISHED;
}

void handle_remove(void)
{
  switch (command_state) {
    case START:
    {
      // DDB info is queried on startup
      sl_status_t sc = btmesh_prov_list_provisioned_nodes();
      if (SL_STATUS_EMPTY == sc) {
        ESP_LOGW(TAG, "No provisioned nodes available");
        command_state = FINISHED;
      }  
      uint16_t id = 0;        
      sc = btmesh_prov_remove_node_by_id(id, remove_node_job_status_ui, &remove_uuid);
      if (SL_STATUS_OK != sc) {
        ESP_LOGE(TAG, "remove node FAIL 0x%04x", sc);
        command_state = FINISHED;
      }        
      command_state = IN_PROGRESS;
    } break;
    case IN_PROGRESS:
      // Wait for confirmation callback
      break;
    case FINISHED:
      command_state = INIT;
      // exit(EXIT_SUCCESS);
      break;
    default:
      // handle_ui_remove();
      break;
  }
}

void handle_reset(void)
{
  switch (command_state) {
    case START:
      ESP_LOGI(TAG, "Initiating node reset");
      sl_btmesh_node_reset();
      // Timer to let the NVM clear properly
      sl_reset_timer_start();
      command_state = IN_PROGRESS;
      break;
    case IN_PROGRESS:
      // Do nothing, timer callback will trigger next state
      break;
    case FINISHED:
      ESP_LOGI(TAG, "Resetting hardware");
      // Reset the hardware
      sl_bt_system_reset(0);
      // exit(EXIT_SUCCESS);
      break;
    default:
      // handle_ui_reset();
      break;
  }
}

void app_parse_uuid(char *input, size_t length, uuid_128 *parsed_uuid)
{
  // Sanity check, do nothing if length is incorrect
  if (UUID_LEN_WITHOUT_SEPARATORS == length
      || UUID_LEN_WITH_SEPARATORS == length) {
    for (size_t count = 0; count < 16; count++) {
      sscanf(input, "%2hhx", &parsed_uuid->data[count]);
      // If data is in AA:BB:CC format move the pointer by 3 bytes and
      // skip the separator, otherwise move by 2 bytes
      input += (UUID_LEN_WITHOUT_SEPARATORS == length ? 2 : 3);
    }
  }
}

void app_parse_address(char *input, size_t length, uint16_t *address)
{
  // Sanity check, do nothing if length is incorrect
  if (ADDRESS_LEN_WITHOUT_PREFIX == length
      || ADDRESS_LEN_WITH_PREFIX == length) {
    if (ADDRESS_LEN_WITH_PREFIX == length) {
      // Discard prefix if address is in 0x1234 format
      input += 2;
    }
    sscanf(input, "%4hx", address);
  }
}

void app_parse_mac(char *input, size_t length, bd_addr *mac)
{
  // Sanity check, do nothing if length is incorrect
  if (MAC_LEN_WITHOUT_SEPARATORS == length
      || MAC_LEN_WITH_SEPARATORS == length) {
    // bd_addr is in reverse byte order
    for (size_t count = 5; count >= 0; count--) {
      sscanf(input, "%2hhx", &mac->addr[count]);
      input += (12 == length ? 2 : 3);
    }
  }
}

// -----------------------------------------------------------------------------
// Callbacks

void app_on_nodeinfo_end(void)
{
  if (command_state < UI_START) {
    command_state = FINISHED;
  }
}

void btmesh_prov_on_node_found_evt(uint8_t bearer,
                                   bd_addr address,
                                   uuid_128 uuid,
                                   int8_t rssi)
{
  ESP_LOGI(TAG, "Bearer:  %s", bearer == 1 ? "PB_GATT" : "PB_ADV");
  ESP_LOGI(TAG, "MAC:     %02x %02x %02x %02x %02x %02x",
               address.addr[5],
               address.addr[4],
               address.addr[3],
               address.addr[2],
               address.addr[1],
               address.addr[0]);
  ESP_LOGI(TAG, "UUID:    %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
               uuid.data[0],
               uuid.data[1],
               uuid.data[2],
               uuid.data[3],
               uuid.data[4],
               uuid.data[5],
               uuid.data[6],
               uuid.data[7],
               uuid.data[8],
               uuid.data[9],
               uuid.data[10],
               uuid.data[11],
               uuid.data[12],
               uuid.data[13],
               uuid.data[14],
               uuid.data[15]);
  ESP_LOGI(TAG, "RSSI:    %d", rssi);
}

void btmesh_prov_on_device_provisioned_evt(uint16_t address, uuid_128 uuid)
{
  ESP_LOGI(TAG, "Device provisioned");
  ESP_LOGI(TAG, "UUID:    %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
               uuid.data[0],
               uuid.data[1],
               uuid.data[2],
               uuid.data[3],
               uuid.data[4],
               uuid.data[5],
               uuid.data[6],
               uuid.data[7],
               uuid.data[8],
               uuid.data[9],
               uuid.data[10],
               uuid.data[11],
               uuid.data[12],
               uuid.data[13],
               uuid.data[14],
               uuid.data[15]);
  ESP_LOGI(TAG, "Address: 0x%04x", address);
  if (UI_START > command_state) {
    // If oneshot mode is used (no UI) then the configuration shall be started
    // here otherwise the configuration is started in the UI event handler
    sl_status_t sc = app_conf_start_node_configuration(APP_NETKEY_IDX,
                                                       address);
    if (SL_STATUS_OK != sc) {
      ESP_LOGE(TAG, "Failed to start configuration procedure.");
      command_state = FINISHED;
    }
  }
}

void app_on_node_configuration_end(uint16_t netkey_index,
                                   uint16_t server_address)
{
  if (UI_START > command_state) {
    command_state = FINISHED;
  }
}

void btmesh_prov_on_provision_failed_evt(uint8_t reason, uuid_128 uuid)
{
  if (UI_START > command_state) {
    command_state = FINISHED;
  } else {
    // btmesh_prov_on_provision_failed_evt_ui();
  }
}

void btmesh_prov_on_ddb_list_ready(uint16_t count)
{
  if (0 == count) {
    ESP_LOGD(TAG, "No nodes in DDB");
  } else {
    if (NODELIST == command) {
      // DDB list requested by user
      ESP_LOGD(TAG, "All nodes in DDB listed");
      // Finish only if not requested by UI mode
      if (UI_START > command_state) {
        command_state = FINISHED;
      } else {
        // btmesh_prov_on_ddb_list_ready_ui();
      }
    } else if (REMOVE_NODE == command) {
      // Node removal can only start after the database is ready
      sl_status_t sc = btmesh_prov_remove_node_by_uuid(command_uuid,
                                                       app_on_remove_node_job_status);
      if (SL_STATUS_OK != sc) {
        command_state = FINISHED;
      } else {
        command_state = IN_PROGRESS;
      }
    } else if (NODEINFO == command) {
      // Node information can only be obtained after the database is ready
      ESP_LOGI(TAG, "Querying node information");
      command_state = IN_PROGRESS;
      app_conf_print_nodeinfo_by_uuid(command_uuid);
    }
  }
}

// -----------------------------------------------------------------------------
// Callbacks

void app_on_remove_node_job_status(const btmesh_conf_job_t *job)
{
  if (BTMESH_CONF_JOB_RESULT_SUCCESS == job->result) {
    ESP_LOGI(TAG, "Node removed from network");
    btmesh_prov_delete_ddb_entry(command_uuid);
  } else {
    ESP_LOGE(TAG, "Could not remove node from network");
  }
  command_state = FINISHED;
}

static TimerHandle_t xTimerGeneral[2];

static void sl_simple_timer_create(void)
{
  xTimerGeneral[0] = xTimerCreate("timer_scan", SCAN_TIMER_MS/portTICK_PERIOD_MS, false, (void *)0, app_on_scan_timer);
  xTimerGeneral[1] = xTimerCreate("timer_reset", RESET_TIMER_MS/portTICK_PERIOD_MS, false, (void *)1, app_on_reset_timer);
}

void sl_scan_timer_start(void)
{
  xTimerStart(xTimerGeneral[0], 0);
}

void sl_reset_timer_start(void)
{
  xTimerStart(xTimerGeneral[1], 0);
}

static void app_on_scan_timer(TimerHandle_t xTimer)
{
  command_state = FINISHED;
}

static void app_on_reset_timer(TimerHandle_t xTimer)
{
  command_state = FINISHED;
}

// -----------------------------------------------------------------------------
// Helper functions
void set_command_and_state(command_t new_command, command_state_t new_state)
{
  command_state = new_state;
  command = new_command;
}

// void sl_btmesh_generic_base_on_event(sl_btmesh_msg_t *evt)
// {
// 	 sl_status_t sc = SL_STATUS_OK;
// 	switch (SL_BT_MSG_ID(evt->header))
// 	{
// 		case sl_btmesh_evt_node_initialized_id:
//     sc = sl_btmesh_generic_server_init();
//     app_assert_status_f(sc, "Failed to init Generic Server");
// 		if (evt->data.evt_node_initialized.provisioned) {
//         sl_btmesh_generic_server_init_on_off();
//     }
// 		break;
// 		case sl_btmesh_evt_generic_server_client_request_id:
//     // ESP_LOGI(TAG,"evt_generic_server server_address: %d",evt->data.evt_generic_server_client_request.server_address);  
//     // ESP_LOGI(TAG,"evt_generic_server client_address: %d",evt->data.evt_generic_server_client_request.client_address);
//     // ESP_LOGI(TAG,"evt_generic_server delay_ms: %d",evt->data.evt_generic_server_client_request.delay_ms);
//     // ESP_LOGI(TAG,"evt_generic_server elem_index: %d",evt->data.evt_generic_server_client_request.elem_index);   
//     // ESP_LOGI(TAG,"evt_generic_server model_id: %d",evt->data.evt_generic_server_client_request.model_id);
    
//     // ESP_LOGI(TAG,"evt_generic_server parameters len: %d",evt->data.evt_generic_server_client_request.parameters.len);
//     // for(uint8_t i = 0; i < evt->data.evt_generic_server_client_request.parameters.len; i++)
//     // {
//     //  ESP_LOGI(TAG,"%d",evt->data.evt_generic_server_client_request.parameters.data[i]);
//     // }  
//     // ESP_LOGI(TAG,"evt_generic_server appkey_index: %d",evt->data.evt_generic_server_client_request.appkey_index);
//     // ESP_LOGI(TAG,"evt_generic_server transition_ms: %d",evt->data.evt_generic_server_client_request.transition_ms);
//     // ESP_LOGI(TAG,"evt_generic_server type: %d",evt->data.evt_generic_server_client_request.type);
//     if(evt->data.evt_generic_server_client_request.parameters.data[0] == 0) ESP_LOGI(TAG,"LED OFF");
//     else ESP_LOGI(TAG,"LED ON");
// 		break;
//     case sl_btmesh_evt_generic_server_state_recall_id:
//     ESP_LOGI(TAG,"sl_btmesh_evt_generic_server_state_recall_id");
// 		break;
//     case sl_btmesh_evt_generic_server_state_changed_id:
//       // mesh_lib_generic_server_event_handler(evt);
// 			ESP_LOGI(TAG,"sl_btmesh_evt_generic_server_state_changed_id");
//     break;
// 		case sl_btmesh_evt_node_provisioned_id:
//        sl_btmesh_generic_server_init_on_off();
//     break;


// 		default:
//       break;
// 	}
// }

void sl_btmesh_generic_base_on_event(sl_btmesh_msg_t *evt)
{
	switch (SL_BT_MSG_ID(evt->header))
	{
	  case sl_btmesh_evt_generic_server_client_request_id:
    {
      app_log_info("DA VAO DAY\n");
    } break;
		default:
      break;    
  }
}

void sl_btmesh_common_event(sl_btmesh_msg_t *evt)
{
	sl_status_t sc;
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_btmesh_evt_node_initialized_id:
      if (!evt->data.evt_node_initialized.provisioned) {
        // The Node is now initialized,
        // start unprovisioned Beaconing using PB-ADV and PB-GATT Bearers
        ESP_LOGI(TAG,"Initialized");
        sc = sl_btmesh_node_start_unprov_beaconing(0x3);
        app_assert(sc == SL_STATUS_OK,
                   "[E: 0x%04x] Failed to start unprovisioned beaconing",
                   (int)sc);
      }
      break;
    case sl_btmesh_evt_node_provisioning_started_id:
      ESP_LOGI(TAG,"evt_node_provisioning_started result: %d",evt->data.evt_node_provisioning_started.result);
    break;
    
    case sl_btmesh_evt_health_server_attention_id:
      // ESP_LOGI(TAG,"evt_health_server_attention elem_index: %d",evt->data.evt_health_server_attention.elem_index);
      // ESP_LOGI(TAG,"evt_health_server_attention time_sec: %d",evt->data.evt_health_server_attention.timer_sec);

    break;
    case sl_btmesh_evt_node_key_added_id:
      // ESP_LOGI(TAG,"evt_node_key_added_id index: %d",evt->data.evt_node_key_added.index);  
      // ESP_LOGI(TAG,"evt_node_key_added_id netkey_index: %d",evt->data.evt_node_key_added.netkey_index);
      // ESP_LOGI(TAG,"evt_node_key_added_id type: %d",evt->data.evt_node_key_added.type);
    break;
    case sl_btmesh_evt_node_provisioned_id:
    //  sl_btmesh_generic_server_init_on_off();
     ESP_LOGI(TAG,"evt_node_provisioned_id");
    break;

    case sl_btmesh_evt_proxy_connected_id:
      ESP_LOGI(TAG,"evt_proxy_connected handler: %d",evt->data.evt_proxy_connected.handle);
    break; 
   
    case sl_btmesh_evt_node_config_set_id:
	  	ESP_LOGI(TAG,"evt_node_config_set_id");
      // ESP_LOGI(TAG,"evt_node_config_set_id.id: %d",evt->data.evt_node_config_set.id);
      // ESP_LOGI(TAG,"evt_node_config_set_id.netkey_index: %d",evt->data.evt_node_config_set.netkey_index);
      // ESP_LOGI(TAG,"evt_node_config_set_id.value: ");
      // for(uint8_t i = 0; i < evt->data.evt_node_config_set.value.len; i++)
      // {
      //   ESP_LOGI(TAG,"%x ", evt->data.evt_node_config_set.value.data[i]);
      // }
      // ESP_LOGI(TAG,"");
    break;
    case sl_btmesh_evt_proxy_disconnected_id:
      // ESP_LOGI(TAG,"sl_btmesh_evt_proxy_disconnected_id handler: %d", evt->data.evt_proxy_disconnected.handle);
      // ESP_LOGI(TAG,"sl_btmesh_evt_proxy_disconnected_id reason: %x", evt->data.evt_proxy_disconnected.reason);
      
    break;
    case sl_btmesh_evt_node_model_config_changed_id:
      // ESP_LOGI(TAG,"evt_node_model_config_changed element_address: %d", evt->data.evt_node_model_config_changed.element_address);
      // ESP_LOGI(TAG,"evt_node_model_config_changed model_id: %d", evt->data.evt_node_model_config_changed.model_id);
      // ESP_LOGI(TAG,"evt_node_model_config_changed vendor_id: %d", evt->data.evt_node_model_config_changed.vendor_id);
      // ESP_LOGI(TAG,"evt_node_model_config_changed node_config_state: %d", evt->data.evt_node_model_config_changed.node_config_state);
    break;
     
    case sl_btmesh_evt_node_reset_id:
      ESP_LOGI(TAG,"sl_btmesh_evt_node_reset_id");
    break;
    default:
      // ESP_LOGI(TAG,"mesh id: %x",SL_BT_MSG_ID(evt->header));
      break;
  }
}
/**************************************************************************//**
 * Bluetooth Mesh stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth Mesh stack.
 *****************************************************************************/
// void sl_btmesh_on_event(sl_btmesh_msg_t *evt)
// {
//   sl_btmesh_generic_base_on_event(evt);
//   sl_btmesh_common_event(evt);
// }

void btmesh_prov_on_unprovisioned_node_list_evt(uint16_t id,
                                                uuid_128 uuid,
                                                bd_addr mac)
{
  ESP_LOGI(TAG,"Unprovisioned node" );
  ESP_LOGI(TAG,"ID:      %d", id);
  print_uuid(&uuid);
  print_mac(&mac);
}

void btmesh_prov_on_provisioned_node_list_evt(uint16_t id,
                                              uuid_128 uuid,
                                              uint16_t primary_address)
{
  ESP_LOGI(TAG,"Provisioned node" );
  ESP_LOGI(TAG,"ID:      %d" , id);
  print_uuid(&uuid);
  ESP_LOGI(TAG,"Address: 0x%04x" , primary_address);
}