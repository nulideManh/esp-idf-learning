#include "sl_simple_timer.h"
#include "app_log.h"
// -----------------------------------------------------------------------------
// Definitions



// -----------------------------------------------------------------------------
// Private variables

/// Start of the linked list which contains the active timers.
static sl_simple_timer_t *simple_timer_head = NULL;

/***************************************************************************//**
 * Append a timer to the end of the linked list.
 *
 * @param[in] timer Pointer to the timer handle.
 *
 * @pre Assumes that the timer is not present in the list.
 ******************************************************************************/
static void append_simple_timer(sl_simple_timer_t *timer);

/***************************************************************************//**
 * Remove a timer from the linked list.
 *
 * @param[in] timer Pointer to the timer handle.
 *
 * @return Presence of the timer in the linked list.
 * @retval true  Timer was in the list.
 * @retval false Timer was not found in the list.
 ******************************************************************************/
static bool remove_simple_timer(sl_simple_timer_t *timer);

// -----------------------------------------------------------------------------
// Public function definitions

sl_status_t sl_simple_timer_start(sl_simple_timer_t *timer,
                                  uint32_t timeout_ms,
                                  sl_simple_timer_callback_t callback,
                                  void *callback_data,
                                  bool is_periodic)
{
  sl_status_t sc;
  // Check input parameters.
  if ((timeout_ms == 0) && is_periodic) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  // Make sure that timer is stopped, also check for NULL.
  // sc = sl_simple_timer_stop(timer);
  // if (SL_STATUS_OK != sc) {
  //   return sc;
  // }

  // Start sleeptimer with the given timeout/period.
  if (is_periodic) {
    timer->timerHandle = xTimerCreate("timer", timeout_ms/portTICK_PERIOD_MS, true, callback_data, callback);
    xTimerStart(timer->timerHandle, 0);
  } else {
    timer->timerHandle = xTimerCreate("timer", timeout_ms/portTICK_PERIOD_MS, false, callback_data, callback);
    xTimerStart(timer->timerHandle, 0);
  }

  append_simple_timer(timer);
  return SL_STATUS_OK;
}

sl_status_t sl_simple_timer_stop(sl_simple_timer_t *timer)
{
  bool timer_present;

  if (timer == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  if (timer->timerHandle == NULL) {
    return SL_STATUS_OK;
  }

  xTimerStop(timer->timerHandle, 0);
  xTimerDelete(timer->timerHandle, 0);
  remove_simple_timer(timer);
  return SL_STATUS_OK;
}

static void append_simple_timer(sl_simple_timer_t *timer)
{
  if (simple_timer_head != NULL) {
    sl_simple_timer_t *current = simple_timer_head;
    // Find end of list.
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = timer;
  } else {
    simple_timer_head = timer;
  }
  timer->next = NULL;
}

static bool remove_simple_timer(sl_simple_timer_t *timer)
{
  sl_simple_timer_t *prev = NULL;
  sl_simple_timer_t *current = simple_timer_head;

  // Find timer in list.
  while (current != NULL && current != timer) {
    prev = current;
    current = current->next;
  }

  if (current != timer) {
    // Not found.
    return false;
  }

  if (prev != NULL) {
    prev->next = timer->next;
  } else {
    simple_timer_head = timer->next;
  }
  return true;
}
