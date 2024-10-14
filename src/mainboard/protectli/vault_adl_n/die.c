/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <console/console.h>
#include <soc/gpio.h>
#include <delay.h>
#include <gpio.h>

#define ITE_GPIO_BASE		0xa00
#define ITE_GPIO_PIN(x)		(1 << ((x) % 10))
#define ITE_GPIO_SET(x)		(((x) / 10) - 1)
#define ITE_GPIO_IO_ADDR(x)	(ITE_GPIO_BASE + ITE_GPIO_SET(x))

static void do_beep(uint32_t frequency, uint32_t duration_msec)
{
	uint32_t timer_delay = 1000000 / frequency / 2;
	uint32_t count = (duration_msec * 1000) / (timer_delay * 2);
	uint8_t val = inb(ITE_GPIO_IO_ADDR(41)); /* GP41 drives a MOSFET for PC Speaker */

	for (uint32_t i = 0; i < count; i++)
	{
		outb(val | ITE_GPIO_PIN(41), ITE_GPIO_IO_ADDR(41));
		udelay(timer_delay);
		outb(val & ~ITE_GPIO_PIN(41), ITE_GPIO_IO_ADDR(41));
		udelay(timer_delay);
	}
}

static void beep_and_blink(void)
{
	static uint8_t blink = 0;
	static uint8_t beep_count = 0;

	gpio_set(GPP_B14, blink);
	/* Beep 12 times at most, constant beeps may be annoying */
	if (beep_count < 12) {
		do_beep(800, 300);
		mdelay(200);
		beep_count++;
	} else {
		mdelay(500);
	}

	blink ^= 1;
}

void die_notify(void)
{
	if (ENV_POSTCAR)
		return;

	/* Make SATA LED blink and use PC SPKR */
	gpio_output(GPP_B14, 0);

	while (1) {
		beep_and_blink();
		beep_and_blink();
		beep_and_blink();
		beep_and_blink();
		delay(2);
	}
}
