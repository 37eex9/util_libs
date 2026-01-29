/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <platsupport/mach/timer.h>
#include <stdint.h>
#include <utils/util.h>

#define MAX_TIMEOUT_NS (JH7110_TIMER_MAX_TICKS * (NS_IN_S / JH7110_TIMER_TICKS_PER_SECOND))

void jh7110_timer_enable(jh7110_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->regs->ctrl |= 0x1;
}

void jh7110_timer_disable(jh7110_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->regs->ctrl &= 0xfffffffe;
}

void jh7110_timer_handle_irq(jh7110_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->value_h += 1;

    /* Reading the EIO register acks the IRQ */
    timer->regs->eoi;
}

uint64_t jh7110_timer_get_time(jh7110_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    /* the timer value counts down from the load value */
    uint64_t value_l = (uint64_t)(JH7110_TIMER_MAX_TICKS - timer->regs->value);
    uint64_t value_h = (uint64_t)timer->value_h;

    /* Include unhandled interrupt in value_h */
    if (timer->regs->int_status & 0x1) {
        value_h += 1;
    }

    uint64_t value_ticks = (value_h << 32) | value_l;

    /* convert from ticks to nanoseconds */
    uint64_t value_whole_seconds = value_ticks / JH7110_TIMER_TICKS_PER_SECOND;
    uint64_t value_subsecond_ticks = value_ticks % JH7110_TIMER_TICKS_PER_SECOND;
    uint64_t value_subsecond_ns =
        (value_subsecond_ticks * NS_IN_S) / JH7110_TIMER_TICKS_PER_SECOND;
    uint64_t value_ns = value_whole_seconds * NS_IN_S + value_subsecond_ns;

    return value_ns;
}

void jh7110_timer_reset(jh7110_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);

    timer->regs->ctrl |= JH7110_TIMER_MODE_FREE_RUNNING;
    timer->regs->load_count = JH7110_TIMER_MAX_TICKS;
    timer->value_h = 0;
}

int jh7110_timer_set_timeout(jh7110_timer_t *timer, uint64_t ns, bool is_periodic)
{
    jh7110_timer_disable(timer);
    timer->value_h = 0;

    if (is_periodic) {
        timer->regs->ctrl = JH7110_TIMER_MODE_FREE_RUNNING;
    } else {
        timer->regs->ctrl = JH7110_TIMER_MODE_USER_DEFINED;
    }

    uint64_t ticks_whole_seconds = (ns / NS_IN_S) * JH7110_TIMER_TICKS_PER_SECOND;
    uint64_t ticks_remainder = (ns % NS_IN_S) * JH7110_TIMER_TICKS_PER_SECOND / NS_IN_S;
    uint64_t num_ticks = ticks_whole_seconds + ticks_remainder;

    if (num_ticks > JH7110_TIMER_MAX_TICKS) {
        ZF_LOGE("Requested timeout of %"PRIu64" ns exceeds hardware limit of %"PRIu64" ns",
                ns,
                MAX_TIMEOUT_NS);
        return -EINVAL;
    }

    timer->regs->load_count = num_ticks;
    jh7110_timer_enable(timer);

    return 0;
}

void jh7110_timer_disable_all(void *vaddr)
{
    assert(vaddr);

    for (int i = 0; i < JH7110_NUM_TIMERS; i++) {
        volatile jh7110_timer_regs_t *regs = vaddr + i * 0x14;
        regs->ctrl &= 0xfffffffe;
    }
}

void jh7110_timer_init(jh7110_timer_t *timer, void *vaddr, size_t n)
{
    assert(timer);
    assert(vaddr);
    assert(n >= 0 && n < JH7110_NUM_TIMERS);

    timer->regs = vaddr + 0x14 * n;
    timer->regs->ctrl = (JH7110_TIMER_DISABLED | JH7110_TIMER_MODE_FREE_RUNNING | JH7110_TIMER_IRQ_UNMASK);
    timer->regs->load_count = JH7110_TIMER_MAX_TICKS;
    timer->value_h = 0;
}
