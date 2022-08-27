/***************************************************************************//**
 * @file
 * @brief NCP host application module without threading.
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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "app_assert.h"
#include "sl_bt_ncp_host.h"
#include "ncp_host.h"

#include "app_uart.h"




static void ncp_host_tx(uint32_t len, uint8_t *data);
static int32_t ncp_host_rx(uint32_t len, uint8_t *data);
static int32_t ncp_host_peek(void);

/**************************************************************************//**
 * Initialize NCP connection.
 *****************************************************************************/
sl_status_t ncp_host_init(void)
{
  uart1_init(APP_UART_TXD,APP_UART_RXD,APP_UART_BAURATE);
  return sl_bt_api_initialize_nonblock(ncp_host_tx, ncp_host_rx, ncp_host_peek);
}


/**************************************************************************//**
 * Deinitialize NCP connection.
 *****************************************************************************/
void ncp_host_deinit(void)
{
  // if (!IS_EMPTY_STRING(uart_port)) {
  //   uartClose(handle_ptr);
  // } 
}

/**************************************************************************//**
 * BGAPI TX wrapper.
 *****************************************************************************/
static void ncp_host_tx(uint32_t len, uint8_t* data)
{
  if (uart1_write_bytes((char *)data,len) < 0) {
    ncp_host_deinit();
    app_assert(false, "Failed to write data\n");
  }
}

/**************************************************************************//**
 * BGAPI RX wrapper.
 *****************************************************************************/
static int32_t ncp_host_rx(uint32_t len, uint8_t* data)
{
  return uart1_read_bytes(data, len);
}

/**************************************************************************//**
 * BGAPI peek wrapper.
 *****************************************************************************/
static int32_t ncp_host_peek(void)
{
  int32_t sc = 0;

  if (sc < 0) {
    ncp_host_deinit();
    app_assert(false,
               "Peek is not supported in your environment, the program will hang.\n"
               "Please try other OS or environment.\n");
  }
  return sc;
}
