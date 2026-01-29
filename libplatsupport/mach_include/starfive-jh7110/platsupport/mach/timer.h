/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/* These values correspond to the DTS node 'soc/timer@0x2000000' */
#define JH7110_TIMER_BASE 0X2000000
/* The device has a size of 0x10000, but we only need 0x1000 for the driver. */
#define JH7110_TIMER_SIZE 0x1000

/* TODO check defines below */
/* see starfive Visionfive 2 SDK (6.0.0) [0] linux driver [1]
 * linux/drivers/clocksource/timer-jh7110.c
 * [0] https://github.com/starfive-tech/VisionFive2/tree/6.0.0
 * [1] https://github.com/starfive-tech/linux/blob/4cecf169f38eb94b40e307f5f870055e4d9d64f1/drivers/clocksource/timer-jh7110.c
 */
// #define JH7110_TIMER_IRQ 0x159
#define JH7110_TIMER_IRQ 0x3

/* This is 24MHz */
#define JH7110_TIMER_TICKS_PER_SECOND 0x16e3600

#define JH7110_TIMER_MAX_TICKS UINT32_MAX

#define JH7110_NUM_TIMERS 8

#define JH7110_TIMER_ENABLED 0x1
// #define JH7110_TIMER_ENABLED 0x10 /* register */
#define JH7110_TIMER_DISABLED 0x0 /* OK */
#define JH7110_TIMER_MODE_FREE_RUNNING 0x0 /* compare continuous JH7110_TIMER_MODE_CONTIN in linux driver */
#define JH7110_TIMER_MODE_USER_DEFINED (1 << 1) /* compare single in linux driver */
#define JH7110_TIMER_IRQ_UNMASK 0x0
// #define JH7110_TIMER_IRQ_UNMASK 0x1 /* JH7110_TIMER_INT_CLR_AVA_MASK */

typedef struct {
    uint32_t load_count;
    uint32_t value;
    uint32_t ctrl;
    uint32_t eoi;
    uint32_t int_status;
} jh7110_timer_regs_t;

typedef struct {
    volatile jh7110_timer_regs_t *regs;
    /*
     * Stores the number of times the continuous counter timer has elapsed and started over.
     * This allows us to count to a higher number than allowed by the hardware.
     */
    uint32_t value_h;
} jh7110_timer_t;

void jh7110_timer_enable(jh7110_timer_t *timer);
void jh7110_timer_disable(jh7110_timer_t *timer);
void jh7110_timer_handle_irq(jh7110_timer_t *timer);
uint64_t jh7110_timer_get_time(jh7110_timer_t *timer);
void jh7110_timer_reset(jh7110_timer_t *timer);
int jh7110_timer_set_timeout(jh7110_timer_t *timer, uint64_t ns, bool is_periodic);
void jh7110_timer_disable_all(void *vaddr);
void jh7110_timer_init(jh7110_timer_t *timer, void *vaddr, uint64_t channel);
