/*******************************************************************************
 *
 * Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
*******************************************************************************/
/******************************************************************************/
/**
 *
 * @file xdprx_intr_timer_example.c
 *
 * Contains a design example using the XDprx driver with a user-defined hook
 * for delay. The reasoning behind this is that MicroBlaze sleep is not very
 * accurate without a hardware timer. For systems that have a hardware timer,
 * the user may override the default MicroBlaze sleep with a function that will
 * use the hardware timer.
 * Also, this example sets up the interrupt controller and defines handlers for
 * various interrupt types. In this way, the RX interrupt handler will arbitrate
 * the different interrupts to their respective interrupt handlers defined in
 * this example.
 * This example will print out the detected resolution of the incoming
 * DisplayPort video stream.
 * This example is meant to take in the incoming DisplayPort video stream and
 * output it over HDMI (pass-through).
 *
 * @note	This example requires an AXI timer in the system.
 * @note	For this example to work, the user will need to implement
 *		initialization of the system (Dprx_PlatformInit) as this is
 *		system-specific.
 * @note	For this example to display output, the user will need to
 *		implement the Dprx_Vidpipe* functions which configure and
 *		reset the video pipeline as required as this is system-specific.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -----------------------------------------------
 * 1.0   als  01/20/15 Initial creation.
 * </pre>
 *
*******************************************************************************/

/******************************* Include Files ********************************/

#include "xdprx.h"
#include "xparameters.h"
#ifdef XPAR_INTC_0_DEVICE_ID
/* For MicroBlaze systems. */
#include "xintc.h"
#else
/* For ARM/Zynq SoC systems. */
#include "xscugic.h"
#endif /* XPAR_INTC_0_DEVICE_ID */
#include "xtmrctr.h"

/**************************** Constant Definitions ****************************/

/* The following constants map to the XPAR parameters created in the
 * xparameters.h file. */
#define DPRX_DEVICE_ID XPAR_DISPLAYPORT_0_DEVICE_ID
#ifdef XPAR_INTC_0_DEVICE_ID
#define DP_INTERRUPT_ID \
XPAR_PROCESSOR_SUBSYSTEM_INTERCONNECT_AXI_INTC_1_DISPLAYPORT_0_AXI_INT_INTR
#define INTC_DEVICE_ID		XPAR_INTC_0_DEVICE_ID
#else
#define DP_INTERRUPT_ID		XPAR_FABRIC_DISPLAYPORT_0_AXI_INT_INTR
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#endif /* XPAR_INTC_0_DEVICE_ID */

/****************************** Type Definitions ******************************/

/* Depending on whether the system is a MicroBlaze or ARM/Zynq SoC system,
 * different drivers and associated types will be used. */
#ifdef XPAR_INTC_0_DEVICE_ID
#define INTC		XIntc
#define INTC_HANDLER	XIntc_InterruptHandler
#else
#define INTC		XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler
#endif /* XPAR_INTC_0_DEVICE_ID */

/**************************** Function Prototypes *****************************/

u32 Dprx_IntrTimerExample(XDprx *InstancePtr, u16 DeviceId, INTC *IntcPtr,
			u16 IntrId, u16 DpIntrId, XTmrCtr *TimerCounterPtr);
static u32 Dprx_SetupExample(XDprx *InstancePtr, u16 DeviceId);
static u32 Dprx_SetupInterruptHandler(XDprx *InstancePtr, INTC *IntcPtr,
						u16 IntrId, u16 DpIntrId);
static void Dprx_CustomWaitUs(void *InstancePtr, u32 MicroSeconds);
static void Dprx_ResetVideoOutput(void *InstancePtr);
static void Dprx_DetectResolution(void *InstancePtr);
static void Dprx_InterruptHandlerVmChange(void *InstancePtr);
static void Dprx_InterruptHandlerPowerState(void *InstancePtr);
static void Dprx_InterruptHandlerNoVideo(void *InstancePtr);
static void Dprx_InterruptHandlerVBlank(void *InstancePtr);
static void Dprx_InterruptHandlerTrainingLost(void *InstancePtr);
static void Dprx_InterruptHandlerVideo(void *InstancePtr);
static void Dprx_InterruptHandlerTrainingDone(void *InstancePtr);
static void Dprx_InterruptHandlerBwChange(void *InstancePtr);
static void Dprx_InterruptHandlerTp1(void *InstancePtr);
static void Dprx_InterruptHandlerTp2(void *InstancePtr);
static void Dprx_InterruptHandlerTp3(void *InstancePtr);

extern void Dprx_VidpipeConfig(XDprx *InstancePtr);
extern void Dprx_VidpipeReset(void);

/*************************** Variable Declarations ****************************/

XDprx DprxInstance; /* The Dprx instance. */
INTC IntcInstance; /* The interrupt controller instance. */
XTmrCtr TimerCounterInst; /* The timer counter instance. */

/* Used by interrupt handlers. */
u8 VBlankEnable;
u8 VBlankCount;

/**************************** Function Definitions ****************************/

/******************************************************************************/
/**
 * This function is the main function of the XDprx interrupt with timer example.
 * If the DprxIntrTimerExample function, which sets up the system succeeds, this
 * function will wait for interrupts.
 *
 * @param	None.
 *
 * @return
 *		- XST_FAILURE if the interrupt example was unsuccessful - system
 *		  setup failed.
 *
 * @note	Unless setup failed, main will never return since
 *		DprxIntrTimerExample is blocking.
 *
*******************************************************************************/
int main(void)
{
	u32 Status;

	/* Run the XDprx timer example. */
	Status = Dprx_IntrTimerExample(&DprxInstance, DPRX_DEVICE_ID,
				&IntcInstance, INTC_DEVICE_ID, DP_INTERRUPT_ID,
				&TimerCounterInst);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/******************************************************************************/
/**
 * The main entry point for the interrupt with timer example using the XDprx
 * driver. This function will set up the system, interrupt controller and
 * interrupt handlers, and the custom sleep handler.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 * @param	DeviceId is the unique device ID of the DisplayPort RX core
 *		instance.
 * @param	IntcPtr is a pointer to the interrupt instance.
 * @param	IntrId is the unique device ID of the interrupt controller.
 * @param	DpIntrId is the interrupt ID of the DisplayPort RX connection to
 *		the interrupt controller.
 * @param	TimerCounterPtr is a pointer to the timer instance.
 *
 * @return
 *		- XST_SUCCESS if the system was set up correctly and link
 *		  training was successful.
 *		- XST_FAILURE otherwise.
 *
 * @note	None.
 *
*******************************************************************************/
u32 Dprx_IntrTimerExample(XDprx *InstancePtr, u16 DeviceId, INTC *IntcPtr,
			u16 IntrId, u16 DpIntrId, XTmrCtr *TimerCounterPtr)
{
	u32 Status;

	/* Do platform initialization here. This is hardware system specific -
	 * it is up to the user to implement this function. */
	Dprx_PlatformInit(InstancePtr);
	/*******************/

	/* Set a custom timer handler for improved delay accuracy on MicroBlaze
	 * systems since the driver does not assume/have a dependency on the
	 * system having a timer in the FPGA.
	 * Note: This only has an affect for MicroBlaze systems since the Zynq
	 * ARM SoC contains a timer, which is used when the driver calls the
	 * delay function. */
	XDprx_SetUserTimerHandler(InstancePtr, &Dprx_CustomWaitUs,
							TimerCounterPtr);

	/* Setup interrupt handling in the system. */
	Status = Dprx_SetupInterruptHandler(InstancePtr, IntcPtr, IntrId,
								DpIntrId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = Dprx_SetupExample(InstancePtr, DeviceId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Do not return in order to allow interrupt handling to run. */
	while (1);

	return XST_SUCCESS;
}

/******************************************************************************/
/**
 * This function will setup and initialize the DisplayPort RX core. The core's
 * configuration parameters will be retrieved based on the configuration
 * to the DisplayPort RX core instance with the specified device ID.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 * @param	DeviceId is the unique device ID of the DisplayPort RX core
 *		instance.
 *
 * @return
 *		- XST_SUCCESS if the device configuration was found and obtained
 *		  and if the main link was successfully established.
 *		- XST_FAILURE otherwise.
 *
 * @note	None.
 *
*******************************************************************************/
static u32 Dprx_SetupExample(XDprx *InstancePtr, u16 DeviceId)
{
	XDp_Config *ConfigPtr;
	u32 Status;

	/* Obtain the device configuration for the DisplayPort RX core. */
	ConfigPtr = XDp_LookupConfig(DeviceId);
	if (!ConfigPtr) {
		return XST_FAILURE;
	}
	/* Copy the device configuration into the InstancePtr's Config
	 * structure. */
	XDprx_CfgInitialize(InstancePtr, ConfigPtr, ConfigPtr->BaseAddr);

	XDprx_SetLaneCount(InstancePtr, InstancePtr->Config.MaxLaneCount);
	XDprx_SetLinkRate(InstancePtr, InstancePtr->Config.MaxLinkRate);

	/* Initialize the DisplayPort RX core. */
	Status = XDprx_InitializeRx(InstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/******************************************************************************/
/**
 * This function sets up the interrupt system such that interrupts caused by
 * Hot-Plug-Detect (HPD) events and pulses are handled. This function is
 * application-specific for systems that have an interrupt controller connected
 * to the processor. The user should modify this function to fit the
 * application.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 * @param	IntcPtr is a pointer to the interrupt instance.
 * @param	IntrId is the unique device ID of the interrupt controller.
 * @param	DpIntrId is the interrupt ID of the DisplayPort RX connection to
 *		the interrupt controller.
 *
 * @return
 *		- XST_SUCCESS if the interrupt system was successfully set up.
 *		- XST_FAILURE otherwise.
 *
 * @note	An interrupt controller must be present in the system, connected
 *		to the processor and the DisplayPort RX core.
 *
*******************************************************************************/
static u32 Dprx_SetupInterruptHandler(XDprx *InstancePtr, INTC *IntcPtr,
						u16 IntrId, u16 DpIntrId)
{
	u32 Status;

	/* Set the HPD interrupt handlers. */
	XDprx_SetIntrVmChangeHandler(InstancePtr,
			Dprx_InterruptHandlerVmChange, InstancePtr);
	XDprx_SetIntrPowerStateHandler(InstancePtr,
			Dprx_InterruptHandlerPowerState, InstancePtr);
	XDprx_SetIntrNoVideoHandler(InstancePtr,
			Dprx_InterruptHandlerNoVideo, InstancePtr);
	XDprx_SetIntrVBlankHandler(InstancePtr,
			Dprx_InterruptHandlerVBlank, InstancePtr);
	XDprx_SetIntrTrainingLostHandler(InstancePtr,
			Dprx_InterruptHandlerTrainingLost, InstancePtr);
	XDprx_SetIntrVideoHandler(InstancePtr,
			Dprx_InterruptHandlerVideo, InstancePtr);
	XDprx_SetIntrTrainingDoneHandler(InstancePtr,
			Dprx_InterruptHandlerTrainingDone, InstancePtr);
	XDprx_SetIntrBwChangeHandler(InstancePtr,
			Dprx_InterruptHandlerBwChange, InstancePtr);
	XDprx_SetIntrTp1Handler(InstancePtr,
			Dprx_InterruptHandlerTp1, InstancePtr);
	XDprx_SetIntrTp2Handler(InstancePtr,
			Dprx_InterruptHandlerTp2, InstancePtr);
	XDprx_SetIntrTp3Handler(InstancePtr,
			Dprx_InterruptHandlerTp3, InstancePtr);

	/* Initialize interrupt controller driver. */
#ifdef XPAR_INTC_0_DEVICE_ID
	Status = XIntc_Initialize(IntcPtr, IntrId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
#else
	XScuGic_Config *IntcConfig;

	IntcConfig = XScuGic_LookupConfig(IntrId);
	Status = XScuGic_CfgInitialize(IntcPtr, IntcConfig,
						IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	XScuGic_SetPriorityTriggerType(IntcPtr, DpIntrId, 0xA0, 0x1);
#endif /* XPAR_INTC_0_DEVICE_ID */

	/* Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device. */
#ifdef XPAR_INTC_0_DEVICE_ID
	Status = XIntc_Connect(IntcPtr, DpIntrId,
			(XInterruptHandler)XDprx_InterruptHandler, InstancePtr);
#else
	Status = XScuGic_Connect(IntcPtr, DpIntrId,
		(Xil_InterruptHandler)XDprx_InterruptHandler, InstancePtr);
#endif /* XPAR_INTC_0_DEVICE_ID */
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Start the interrupt controller. */
#ifdef XPAR_INTC_0_DEVICE_ID
	Status = XIntc_Start(IntcPtr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	XIntc_Enable(IntcPtr, DpIntrId);
#else
	XScuGic_Enable(IntcPtr, DpIntrId);
#endif /* XPAR_INTC_0_DEVICE_ID */

	/* Initialize the exception table. */
	Xil_ExceptionInit();

	/* Register the interrupt controller handler with the exception
	 * table. */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)INTC_HANDLER, IntcPtr);

	/* Enable exceptions. */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

/******************************************************************************/
/**
 * This function is used to override the driver's default sleep functionality.
 * For MicroBlaze systems, the XDprx_WaitUs driver function's default behavior
 * is to use the MB_Sleep function from microblaze_sleep.h, which is implemented
 * in software and only has millisecond accuracy. For this reason, using a
 * hardware timer is preferrable. For ARM/Zynq SoC systems, the SoC's timer is
 * used - XDprx_WaitUs will ignore this custom timer handler.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	Use the XDprx_SetUserTimerHandler driver function to set this
 *		function as the handler for when the XDprx_WaitUs driver
 *		function is called.
 *
*******************************************************************************/
static void Dprx_CustomWaitUs(void *InstancePtr, u32 MicroSeconds)
{
	XDprx *XDprx_InstancePtr = (XDprx *)InstancePtr;
	u32 TimerVal;

	XTmrCtr_Start(XDprx_InstancePtr->UserTimerPtr, 0);

	/* Wait specified number of useconds. */
	do {
		TimerVal = XTmrCtr_GetValue(XDprx_InstancePtr->UserTimerPtr, 0);
	}
	while (TimerVal < (MicroSeconds *
			(XDprx_InstancePtr->Config.SAxiClkHz / 1000000)));

	XTmrCtr_Stop(XDprx_InstancePtr->UserTimerPtr, 0);
}

/******************************************************************************/
/**
 * This function is used to reset video output for this example.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_ResetVideoOutput(void *InstancePtr)
{
	xil_printf("\tDisabling the display timing generator.\n");
	XDprx_DtgDis(InstancePtr);

	xil_printf("\tResetting the video output pipeline.\n");
	/* This is hardware system specific - it is up to the user to implement
	 * this function if needed. */
	Dprx_VidpipeReset();
	/*******************/

	xil_printf("\tConfiguring the video output pipeline.\n");
	/* This is hardware system specific - it is up to the user to implement
	 * this function if needed. */
	Dprx_VidpipeConfig(InstancePtr);
	/*******************/

	xil_printf("\tRe-enabling the display timing generator.\n");
	XDprx_DtgEn(InstancePtr);
}

/******************************************************************************/
/**
 * This function will present the resolution of the incoming video stream.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	The resolution will be rounded up to the nearest resolution
 *		present in the XVidC_VideoTimingModes table.
 *
*******************************************************************************/
static void Dprx_DetectResolution(void *InstancePtr)
{
	u32 DpHres, DpVres;
	u32 GetResCount = 0;

	do {
		DpHres = (XDprx_ReadReg(((XDprx *)InstancePtr)->Config.BaseAddr,
							XDPRX_MSA_HRES));
		DpVres = (XDprx_ReadReg(((XDprx *)InstancePtr)->Config.BaseAddr,
							XDPRX_MSA_VHEIGHT));
		GetResCount++;
		XDprx_WaitUs(InstancePtr, 1000);
	} while (((DpHres == 0) || (DpVres == 0)) && (GetResCount < 2000));

	xil_printf("\n*** Detected resolution: %d x %d ***\n", DpHres, DpVres);
}

/******************************************************************************/
/**
 * This function is the callback function for when a video mode chang interrupt
 * occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerVmChange(void *InstancePtr)
{
	u32 Status;

	xil_printf("> Interrupt: video mode change.\n");

	Status = XDprx_CheckLinkStatus(InstancePtr);
	if ((Status == XST_SUCCESS) && (VBlankCount >= 20)) {
		Dprx_ResetVideoOutput(InstancePtr);
		Dprx_DetectResolution(InstancePtr);
	}
}

/******************************************************************************/
/**
 * This function is the callback function for when the power state interrupt
 * occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerPowerState(void *InstancePtr)
{
	xil_printf("> Interrupt: power state change request.\n");
}

/******************************************************************************/
/**
 * This function is the callback function for when a no video interrupt occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerNoVideo(void *InstancePtr)
{
	xil_printf("> Interrupt: no-video flags in the VBID field after active "
						"video has been received.\n");
}

/******************************************************************************/
/**
 * This function is the callback function for when a vertical blanking interrupt
 * occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerVBlank(void *InstancePtr)
{
	u32 Status;

	if (VBlankEnable) {
		VBlankCount++;

		xil_printf("> Interrupt: vertical blanking (frame %d).\n",
								VBlankCount);

		/* Wait until 20 frames have been received before forwarding or
		 * outputting any video stream. */
		if (VBlankCount >= 20) {
			VBlankEnable = 0;
			XDprx_InterruptDisable(InstancePtr,
					XDPRX_INTERRUPT_MASK_VBLANK_MASK);

			Status = XDprx_CheckLinkStatus(InstancePtr);
			if (Status == XST_SUCCESS) {
				Dprx_ResetVideoOutput(InstancePtr);
				Dprx_DetectResolution(InstancePtr);
			}
		}
	}
}

/******************************************************************************/
/**
 * This function is the callback function for when a training lost interrupt
 * occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerTrainingLost(void *InstancePtr)
{
	xil_printf("> Interrupt: training lost.\n");

	/* Re-enable vertical blanking interrupt and counter. */
	VBlankEnable = 1;
	XDprx_InterruptEnable(InstancePtr, XDPRX_INTERRUPT_MASK_VBLANK_MASK);
	VBlankCount = 0;

	xil_printf("\tDisabling the display timing generator.\n");
	XDprx_DtgDis(InstancePtr);
	xil_printf("\tResetting the video output pipeline.\n");
	/* This is hardware system specific - it is up to the user to implement
	 * this function if needed. */
	Dprx_VidpipeReset();
	/*******************/
}

/******************************************************************************/
/**
 * This function is the callback function for when a valid video interrupt
 * occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerVideo(void *InstancePtr)
{
	xil_printf("> Interrupt: a valid video frame is detected on main "
								"link.\n");
}

/******************************************************************************/
/**
 * This function is the callback function for when a training done interrupt
 * occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerTrainingDone(void *InstancePtr)
{
	xil_printf("> Interrupt: training done.\n");
}

/******************************************************************************/
/**
 * This function is the callback function for when a bandwidth change interrupt
 * occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerBwChange(void *InstancePtr)
{
	xil_printf("> Interrupt: bandwidth change.\n");
}

/******************************************************************************/
/**
 * This function is the callback function for when a training pattern 1
 * interrupt occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerTp1(void *InstancePtr)
{
	xil_printf("> Interrupt: training pattern 1.\n");
}

/******************************************************************************/
/**
 * This function is the callback function for when a training pattern 2
 * interrupt occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerTp2(void *InstancePtr)
{
	xil_printf("> Interrupt: training pattern 2.\n");
}

/******************************************************************************/
/**
 * This function is the callback function for when a training pattern 3
 * interrupt occurs.
 *
 * @param	InstancePtr is a pointer to the XDprx instance.
 *
 * @return	None.
 *
 * @note	None.
 *
*******************************************************************************/
static void Dprx_InterruptHandlerTp3(void *InstancePtr)
{
	xil_printf("> Interrupt: training pattern 3.\n");
}