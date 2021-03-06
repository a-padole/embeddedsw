/*
 * Copyright (C) 2014 - 2015 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 */

/*********************************************************************
 * Definitions of commonly used macros and enums in PMU Power
 * Management (PM).
 *********************************************************************/

#ifndef PM_COMMON_H_
#define PM_COMMON_H_

#include "pmu_local.h"
#include "xpfw_default.h"
#include "xil_types.h"

/*********************************************************************
 * Typedefs (common use in PMU Power Management)
 ********************************************************************/
/*
 * stdint.h is not available, and pmu-fw framework defined similar
 * macros but as 0 and 1 (signed). PM, as in general, uses unsigned.
 */
#ifndef bool
	typedef u8 bool;
	#define true	1U
	#define false	0U
#endif

typedef u32 (*const PmTranHandler)(void);

/*********************************************************************
 * Macros
 ********************************************************************/
/*
 * Undefine DEBUG_PM macro to disable power management prints
 */
#ifdef DEBUG_MODE
	#define DEBUG_PM
#endif
/*
 * Conditional debugging prints used for PM. PM prints should never
 * appear if pmu-fw has disabled debug prints, however even when pmu-fw
 * has enabled debug prints, power management prints should be configurable.
 */
#ifdef DEBUG_PM
	#define PmDbg(MSG, ...)	fw_printf("PMUFW: %s: " MSG, __func__, ##__VA_ARGS__)
#else
	#define PmDbg(MSG, ...) {}
#endif

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))

/* Enable/disable macros for wake events in GPI1 register */
#define ENABLE_WAKE(mask)   XPfw_RMW32(PMU_LOCAL_GPI1_ENABLE, mask, mask);
#define DISABLE_WAKE(mask)  XPfw_RMW32(PMU_LOCAL_GPI1_ENABLE, mask, ~(mask));

/* Macros for IPI responses (return values and callbacks) */
#define IPI_RESPONSE1(buf, arg0)				\
	XPfw_Write32((buf) + IPI_BUFFER_RESP_OFFSET, arg0);
#define IPI_RESPONSE2(buf, arg0, arg1)				\
	IPI_RESPONSE1(buf, arg0);				\
	XPfw_Write32((buf) + IPI_BUFFER_RESP_OFFSET + PAYLOAD_ELEM_SIZE, arg1);
#define IPI_RESPONSE3(buf, arg0, arg1, arg2)			\
	IPI_RESPONSE2(buf, arg0, arg1);				\
	XPfw_Write32((buf) + IPI_BUFFER_RESP_OFFSET +		\
		     2 * PAYLOAD_ELEM_SIZE, arg2);
#define IPI_RESPONSE4(buf, arg0, arg1, arg2, arg3)		\
	IPI_RESPONSE3(buf, arg0, arg1, arg2);			\
	XPfw_Write32((buf) + IPI_BUFFER_RESP_OFFSET +		\
		     3 * PAYLOAD_ELEM_SIZE, arg3);
#define IPI_RESPONSE5(buf, arg0, arg1, arg2, arg3, arg4)	\
	IPI_RESPONSE4(buf, arg0, arg1, arg2, arg3);		\
	XPfw_Write32((buf) + IPI_BUFFER_RESP_OFFSET +		\
		     4 * PAYLOAD_ELEM_SIZE, arg4);

/* PMU internal capabilities used in definition of slaves' states */
#define PM_CAP_POWER        0x8U

/* Default transition latencies in us */
#define PM_DEFAULT_LATENCY 1000U
#define PM_POWER_ISLAND_LATENCY 2000U
#define PM_POWER_DOMAIN_LATENCY 10000U

/* Power consumptions of the slave components */
#define DEFAULT_POWER_ON		100U
#define DEFAULT_POWER_RETENTION		50U
#define DEFAULT_POWER_OFF		0U

#define POWER_MISSING			~0U

/*********************************************************************
 * Structure definitions
 ********************************************************************/
/**
 * PmRegisterContext - A pair of address/value used for saving/restoring context
 *                     of a register
 * @addr        Address of a register
 * @value       Variable to store register content
 */
typedef struct PmRegisterContext {
	const u32 addr;
	u32 value;
} PmRegisterContext;

/*********************************************************************
 * Function declarations
 ********************************************************************/
#ifdef DEBUG_PM
const char* PmStrNode(const u32 node);
const char* PmStrMaster(const u32 master);
const char* PmStrAck(const u32 ack);
const char* PmStrReason(const u32 reason);
#endif

#endif /* PM_COMMON_H_ */
