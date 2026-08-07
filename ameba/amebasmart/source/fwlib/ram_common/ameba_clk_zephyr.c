/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr-facing extensions to the AmebaSmart clock driver.
 *
 * The AmebaSmart SDK's ROM exposes RCC_PeriphClockCmd() but not
 * RCC_PeriphClockEnableChk() (the AmebaDPlus HAL provides the latter). Zephyr's
 * clock_control_ameba driver uses it to implement clock_control_get_status().
 * Provide a small implementation that decodes the AmebaSmart clock encoding.
 *
 * APBPeriph_*_CLOCK values pack the target register into bits[31:30]:
 *   0 -> LSYS CKE_GRP0
 *   1 -> LSYS CKE_GRP1
 *   2 -> HSYS HP_CKE
 *   3 -> AON CLK
 * bits[29:0] hold a single-bit mask within the target register.
 */

#include "ameba_soc.h"

u8 RCC_PeriphClockEnableChk(u32 APBPeriph_Clock_in)
{
	u32 group = (APBPeriph_Clock_in >> 30) & 0x3;
	u32 mask = APBPeriph_Clock_in & ~(BIT(31) | BIT(30));
	u32 base;
	u32 offset;
	u32 val;

	switch (group) {
	case 0:
		base = SYSTEM_CTRL_BASE_LP;
		offset = REG_LSYS_CKE_GRP0;
		break;
	case 1:
		base = SYSTEM_CTRL_BASE_LP;
		offset = REG_LSYS_CKE_GRP1;
		break;
	case 2:
		base = SYSTEM_CTRL_BASE_HP;
		offset = REG_HSYS_HP_CKE;
		break;
	case 3:
		base = SYSTEM_CTRL_BASE_LP;
		offset = REG_AON_CLK;
		break;
	default:
		return 0;
	}

	val = HAL_READ32(base, offset);
	return (val & mask) ? 1 : 0;
}
