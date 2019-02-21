/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <linux/arm-smccc.h>
#include <linux/psci.h>
#include <linux/io.h>

#define SHIFT_U32(v, shift)   ((v) << (shift))

/* GPT register definitions */
#define GPT_CR                                   0x0
#define GPT_PR                                   0x4
#define GPT_SR                                   0x8
#define GPT_IR                                   0xc
#define GPT_OCR1                                 0x10
#define GPT_OCR2                                 0x14
#define GPT_OCR3                                 0x18
#define GPT_ICR1                                 0x1c
#define GPT_ICR2                                 0x20
#define GPT_CNT                                  0x24

#define GPT_CR_EN                                BIT(0)
#define GPT_CR_ENMOD                             BIT(1)
#define GPT_CR_DBGEN                             BIT(2)
#define GPT_CR_WAITEN                            BIT(3)
#define GPT_CR_DOZEEN                            BIT(4)
#define GPT_CR_STOPEN                            BIT(5)
#define GPT_CR_CLKSRC_24M                        SHIFT_U32(0x5, 6)
#define GPT_CR_CLKSRC_32K                        SHIFT_U32(0x4, 6)
#define GPT_CR_FRR                               BIT(9)
#define GPT_CR_EN_24M                            BIT(10)
#define GPT_CR_SWR                               BIT(15)

#define GPT_IR_OF1IE                             BIT(0)

#define GPT_USE_32K
#ifdef GPT_USE_32K
#define GPT_FREQ 32768
#define GPT_PRESCALER24M 0
#define GPT_PRESCALER 0
#else
#define GPT_FREQ 1000000
#define GPT_PRESCALER24M (12 - 1)
#define GPT_PRESCALER (2 - 1)
#endif

#define GPT1_IRQ	(55 + 32)

#define write32 writel
#define read32 readl

extern void psci_resume(u32 context_id);
extern void psci_cpu_on_handler(u32 context_id);

#define GIC_BASE                0x31000000
#define GIC_SIZE                0x8000
#define GICC_OFFSET             0x2000
#define GICD_OFFSET             0x1000
#define GICD_BASE		(GIC_BASE + GICD_OFFSET)
#define NUM_INTS_PER_REG	32
#define GICD_ISENABLER(n)       (0x100 + (n) * 4)
static void __maybe_unused gic_it_enable(size_t it)
{
        size_t idx = it / NUM_INTS_PER_REG;
        uint32_t mask = 1 << (it % NUM_INTS_PER_REG);

        /* Enable the interrupt */
        write32(mask, GICD_BASE + GICD_ISENABLER(idx));
}

static uint32_t gpt_base = GPT1_BASE_ADDR;
static void gpt_init(void)
{
	uint32_t val;

	/* Disable GPT */
	write32(0, gpt_base + GPT_CR);

	/* Software reset */
	write32(GPT_CR_SWR, gpt_base + GPT_CR);

	/* Wait for reset bit to clear */
	while ((read32(gpt_base + GPT_CR) & GPT_CR_SWR) != 0);

	/* Set prescaler to target frequency */
	val = ((GPT_PRESCALER24M & 0xf) << 12) |  (GPT_PRESCALER & 0xfff);
	write32(val, gpt_base + GPT_PR);

	/* Select clock source */
#ifdef GPT_USE_32K
	val = GPT_CR_CLKSRC_32K;
#else
	val = GPT_CR_CLKSRC_24M;
#endif
	write32(val, gpt_base + GPT_CR);

	val = read32(gpt_base + GPT_CR);
	val |= GPT_CR_EN;
	val |= GPT_CR_ENMOD;
	val |= GPT_CR_STOPEN;
	val |= GPT_CR_WAITEN;
	val |= GPT_CR_DOZEEN;
	val |= GPT_CR_DBGEN;
	val |= GPT_CR_FRR;
#ifndef GPT_USE_32K
	val |= GPT_CR_EN_24M;
#endif
	write32(val, gpt_base + GPT_CR);
}

static void gpt_schedule_interrupt(uint32_t ms)
{
	uint32_t val;

	/* Disable timer */
	val = read32(gpt_base + GPT_CR);
	val &= ~GPT_CR_EN;
	write32(val, gpt_base + GPT_CR);

	/* Disable and acknowledge interrupts */
	write32(0, gpt_base + GPT_IR);
	write32(0x3f, gpt_base + GPT_SR);

	/* Set compare1 register */
	write32(GPT_FREQ / 1000 * ms, gpt_base + GPT_OCR1);

	/* Enable compare interrupt */
	write32(GPT_IR_OF1IE, gpt_base + GPT_IR);

	gic_it_enable(GPT1_IRQ);

	/* Enable timer */
	val |= GPT_CR_EN;
	val |= GPT_CR_ENMOD;
	write32(val, gpt_base + GPT_CR);
}

static bool __maybe_unused gpt_ack_interrupt(void)
{
        uint32_t val;

        val = read32(gpt_base + GPT_SR);

        /* Disable and acknowledge interrupts */
        write32(0, gpt_base + GPT_IR);
        write32(0x3f, gpt_base + GPT_SR);

        return (val & 0x1) != 0;
}



static unsigned long invoke_psci_smc(unsigned long function_id,
                        unsigned long arg0, unsigned long arg1,
                        unsigned long arg2)
{
        struct arm_smccc_res res;

        arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
        return res.a0;
}

static int do_psci_cpu_suspend(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t power_state;
	uint32_t entry;
	uint32_t context_id;

	power_state = simple_strtoul(argv[1], NULL, 16);

	gpt_init();
	gpt_schedule_interrupt(5000);

	entry = (uint32_t)psci_resume;
	context_id = 0xABCD1234;
	invoke_psci_smc(PSCI_0_2_FN_CPU_SUSPEND, power_state,
                         entry, context_id);

	return 0;
}

static int do_psci_cpu_on(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t core_idx;
	uint32_t entry;
	uint32_t context_id;

	core_idx = simple_strtoul(argv[1], NULL, 16);

	entry = (uint32_t)psci_cpu_on_handler;
	context_id = 0xfeed;
	invoke_psci_smc(PSCI_0_2_FN_CPU_ON, core_idx,
                         entry, context_id);

	return 0;
}

static int do_wfi(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dsb();
	wfi();
	return 0;
}

U_BOOT_CMD(
	psci_cpu_suspend, 2, 1, do_psci_cpu_suspend,
	"Invoke PSCI CPU suspend",
	"power_state\n"
	"    - The power_state to be passed in the call"
);

U_BOOT_CMD(
	psci_cpu_on, 2, 1, do_psci_cpu_on,
	"Turn on secondary core",
	"core_idx\n"
	"    - The index of the core to turn on"
);

U_BOOT_CMD(
	wfi, 1, 1, do_wfi,
	"Execute wfi() instruction",
	"executes WFI on processor"
)

