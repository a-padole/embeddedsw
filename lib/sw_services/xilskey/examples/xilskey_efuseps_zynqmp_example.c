/******************************************************************************
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
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*****************************************************************************/
/**
*
* @file xilskey_efuseps_zynqmp_example.c
* This file illustrates how to program ZynqMp efuse and read back the keys from
* efuse.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- ------------------------------------------------------
* 4.0   vns     10/01/15 First release
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "xilskey_efuseps_zynqmp_input.h"
#include "xil_printf.h"

/***************** Macros (Inline Functions) Definitions *********************/
#define XSK_EFUSEPS_AES_KEY_STRING_LEN			(64)
#define XSK_EFUSEPS_USER_KEY_STRING_LEN			(64)
#define XSK_EFUSEPS_PPK_SHA3_HASH_STRING_LEN_96		(96)
#define XSK_EFUSEPS_PPK_SHA2_HASH_STRING_LEN_64		(64)
#define XSK_EFUSEPS_SPK_ID_STRING_LEN			(8)
#define XSK_EFUSEPS_JTAG_USER_CODE_STRING_LEN		(8)

#define XSK_EFUSEPS_AES_KEY_LEN_IN_BITS			(256)
#define XSK_EFUSEPS_USER_KEY_LEN_IN_BITS		(256)
#define XSK_EFUSEPS_PPK_SHA3HASH_LEN_IN_BITS_384	(384)
#define XSK_EFUSEPS_PPK_SHA2HASH_LEN_IN_BITS_256	(256)
#define XSK_EFUSEPS_SPKID_LEN_IN_BITS			(32)
#define XSK_EFUSEPS_JTAG_USER_CODE_LEN_IN_BITS		(32)

#define XSK_EFUSEPS_RD_FROM_CACHE				(0)
#define XSK_EFUSEPS_RD_FROM_EFUSE				(1)

/**************************** Type Definitions *******************************/


/************************** Function Prototypes ******************************/

static inline u32 XilSKey_EfusePs_ZynqMp_InitData(
				XilSKey_ZynqMpEPs *PsInstancePtr);
static inline u32 XilSKey_EfusePs_Example_ReadSecCtrlBits();

/*****************************************************************************/

int main()
{

	XilSKey_ZynqMpEPs PsInstance = {{0}};
	u32 PsStatus;
	u32 UserKey[8];
	u32 Ppk0[12];
	u32 Ppk1[12];
	u32 SpkId;
	u32 JtagUsrCode;
	s8 Row;
	u32 AesCrc;

#if defined (XSK_XPLAT_ZYNQ) || (XSK_MICROBLAZE_PLATFORM)
	xil_printf("This example will not work for this platform\n\r");
#endif
	/* Initiate the Efuse PS instance */
	PsStatus = XilSKey_EfusePs_ZynqMp_InitData(&PsInstance);
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}

	/* Programming the keys */
	PsStatus = XilSKey_ZynqMp_EfusePs_Write(&PsInstance);
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}

	/* Read keys from cache */
	PsStatus = XilSKey_ZynqMp_EfusePs_CacheLoad();
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}
	xil_printf("keys read from cache \n\r");
	PsStatus = XilSKey_ZynqMp_EfusePs_ReadUserKey(UserKey,
							XSK_EFUSEPS_RD_FROM_CACHE );
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}
	else {
		xil_printf("\n\rUserKey:");
		for (Row = 7; Row >= 0; Row--)
			xil_printf("%08x", UserKey[Row]);
	}
	xil_printf("\n\r");

	PsStatus = XilSKey_ZynqMp_EfusePs_ReadPpk0Hash(Ppk0,
							XSK_EFUSEPS_RD_FROM_CACHE);
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}
	else {
		xil_printf("\n\rPPK0:");
		for (Row = 11; Row >= 0; Row--)
			xil_printf("%08x", Ppk0[Row]);
	}
	xil_printf("\n\r");

	PsStatus = XilSKey_ZynqMp_EfusePs_ReadPpk1Hash(Ppk1,
							XSK_EFUSEPS_RD_FROM_CACHE);
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}
	else {
		xil_printf("\n\rPPK1:");
		for (Row = 11; Row >= 0; Row--)
			xil_printf("%08x", Ppk1[Row]);
	}
	xil_printf("\n\r");

	PsStatus = XilSKey_ZynqMp_EfusePs_ReadSpkId(&SpkId,
							XSK_EFUSEPS_RD_FROM_CACHE);
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}
	else {
		xil_printf("\r\nSpkid: %08x\n\r", SpkId);

	}
	PsStatus = XilSKey_ZynqMp_EfusePs_ReadJtagUsrCode(&JtagUsrCode,
								XSK_EFUSEPS_RD_FROM_CACHE);
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}
	else {
		xil_printf("\r\nJTAGUSER code: %08x\n\r", JtagUsrCode);

	}

	/* CRC check for programmed AES key */
	AesCrc = XilSKey_CrcCalculation((u8 *)XSK_EFUSEPS_AES_KEY);
	PsStatus = XilSKey_ZynqMp_EfusePs_CheckAesKeyCrc(AesCrc);
	if (PsStatus != XST_SUCCESS) {
		xil_printf("\r\nAES CRC checK is failed\n\r");
	}
	else {
		xil_printf("\r\nAES CRC checK is passed\n\r");
	}

	/* Reading control and secure bits of eFuse */
	PsStatus = XilSKey_EfusePs_Example_ReadSecCtrlBits();
	if (PsStatus != XST_SUCCESS) {
		goto EFUSE_ERROR;
	}



EFUSE_ERROR:
	if (PsStatus != XST_SUCCESS) {
		xil_printf("\r\nEfuse example is failed with Status = %08x\n\r",
								PsStatus);
	}
	else {
		xil_printf("\r\nZynqMp Efuse example exited successfully");
	}

	return PsStatus;
}

/****************************************************************************/
/**
*
*
* Helper functions to properly initialize the Ps eFUSE structure instance
*
*
* @param	PsInstancePtr - Structure Address to update the
*		structure elements
*
* @return
*		- XST_SUCCESS - In case of Success
*		- XST_FAILURE - If initialization fails
*
* @note
*
*****************************************************************************/

static inline u32 XilSKey_EfusePs_ZynqMp_InitData(
				XilSKey_ZynqMpEPs *PsInstancePtr)
{

	u32 PsStatus;

	PsStatus = XST_SUCCESS;

	/*
	 * Copy the xilskey_efuseps_zynqmp_input.h values into
	 * PS eFUSE structure elements
	 */

	/* Secure and control bits for programming */
	PsInstancePtr->PrgrmgSecCtrlBits.AesKeyRead = XSK_EFUSEPS_AES_RD_LOCK;
	PsInstancePtr->PrgrmgSecCtrlBits.AesKeyWrite = XSK_EFUSEPS_AES_WR_LOCK;
	PsInstancePtr->PrgrmgSecCtrlBits.UseAESOnly =
						XSK_EFUSEPs_FORCE_USE_AES_ONLY;
	PsInstancePtr->PrgrmgSecCtrlBits.BbramDisable =
						XSK_EFUSEPS_BBRAM_DISABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.PMUError =
					XSK_EFUSEPS_ERR_OUTOF_PMU_DISABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.JtagDisable = XSK_EFUSEPS_JTAG_DISABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.DFTDisable = XSK_EFUSEPS_DFT_DISABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.ProgGate0 =
					XSK_EFUSEPS_PROG_GATE_0_DISABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.ProgGate1 =
					XSK_EFUSEPS_PROG_GATE_1_DISABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.ProgGate2 =
					XSK_EFUSEPS_PROG_GATE_2_DISABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.SecureLock = XSK_EFUSEPS_SECURE_LOCK;
	PsInstancePtr->PrgrmgSecCtrlBits.RSAEnable = XSK_EFUSEPS_RSA_ENABLE;
	PsInstancePtr->PrgrmgSecCtrlBits.PPK0WrLock = XSK_EFUSEPS_PPK0_WR_LOCK;
	PsInstancePtr->PrgrmgSecCtrlBits.PPK0Revoke = XSK_EFUSEPS_PPK0_REVOKE;
	PsInstancePtr->PrgrmgSecCtrlBits.PPK1WrLock = XSK_EFUSEPS_PPK1_WR_LOCK;
	PsInstancePtr->PrgrmgSecCtrlBits.PPK1Revoke = XSK_EFUSEPS_PPK1_REVOKE;

	/* User control bits */
	PsInstancePtr->PrgrmgSecCtrlBits.UserWrLk0 = XSK_EFUSEPS_USER_WRLK_0;
	PsInstancePtr->PrgrmgSecCtrlBits. UserWrLk1 = XSK_EFUSEPS_USER_WRLK_1;
	PsInstancePtr->PrgrmgSecCtrlBits.UserWrLk2 = XSK_EFUSEPS_USER_WRLK_2;
	PsInstancePtr->PrgrmgSecCtrlBits.UserWrLk3 = XSK_EFUSEPS_USER_WRLK_3;
	PsInstancePtr->PrgrmgSecCtrlBits.UserWrLk4 = XSK_EFUSEPS_USER_WRLK_4;
	PsInstancePtr->PrgrmgSecCtrlBits.UserWrLk5 = XSK_EFUSEPS_USER_WRLK_5;
	PsInstancePtr->PrgrmgSecCtrlBits.UserWrLk6 = XSK_EFUSEPS_USER_WRLK_6;
	PsInstancePtr->PrgrmgSecCtrlBits.UserWrLk7 = XSK_EFUSEPS_USER_WRLK_7;

	/* For writing into eFuse */
	PsInstancePtr->PrgrmAesKey = XSK_EFUSEPS_WRITE_AES_KEY;
	PsInstancePtr->PrgrmUserKey = XSK_EFUSEPS_WRITE_USER_KEY;
	PsInstancePtr->PrgrmPpk0Hash = XSK_EFUSEPS_WRITE_PPK0_HASH;
	PsInstancePtr->PrgrmPpk1Hash = XSK_EFUSEPS_WRITE_PPK1_HASH;
	PsInstancePtr->PrgrmSpkID = XSK_EFUSEPS_WRITE_SPKID;
	PsInstancePtr->PrgrmJtagUserCode = XSK_EFUSEPS_WRITE_JTAG_USERCODE;

	/* Variable for Timer Intialization */
	PsInstancePtr->IntialisedTimer = 0;

	/* Copying PPK hash types */
	PsInstancePtr->IsPpk0Sha3Hash = XSK_EFUSEPS_PPK0_IS_SHA3;
	PsInstancePtr->IsPpk1Sha3Hash = XSK_EFUSEPS_PPK1_IS_SHA3;

	/* Copy the keys to be programmed */
	if (PsInstancePtr->PrgrmUserKey == TRUE) {
		/* Validation of User High Key */
		PsStatus = XilSKey_Efuse_ValidateKey(
				(char *)XSK_EFUSEPS_USER_KEY,
				XSK_EFUSEPS_USER_KEY_STRING_LEN);
		if(PsStatus != XST_SUCCESS) {
			goto ERROR;
		}
		/* Assign the User key [255:0]bits */
		XilSKey_Efuse_ConvertStringToHexLE(
			(char *)XSK_EFUSEPS_USER_KEY ,
			&PsInstancePtr->UserKey[0],
			XSK_EFUSEPS_USER_KEY_LEN_IN_BITS);
	}
	if (PsInstancePtr->PrgrmAesKey == TRUE) {
		/* Validation of AES Key */
		PsStatus = XilSKey_Efuse_ValidateKey(
			(char *)XSK_EFUSEPS_AES_KEY,
			XSK_EFUSEPS_AES_KEY_STRING_LEN);
		if(PsStatus != XST_SUCCESS) {
			goto ERROR;
		}
		/* Assign the AES Key Value */
		XilSKey_Efuse_ConvertStringToHexLE(
			(char *)XSK_EFUSEPS_AES_KEY,
			&PsInstancePtr->AESKey[0],
			XSK_EFUSEPS_AES_KEY_LEN_IN_BITS);
	}

	/* Is PPK0 hash programming is enabled */
	if (PsInstancePtr->PrgrmPpk0Hash == TRUE) {
		/* If Sha3 hash is programming into Efuse PPK0 */
		if (PsInstancePtr->IsPpk0Sha3Hash == TRUE) {
			/* Validation of PPK0 sha3 hash */
			PsStatus = XilSKey_Efuse_ValidateKey(
				(char *)XSK_EFUSEPS_PPK0_HASH,
				XSK_EFUSEPS_PPK_SHA3_HASH_STRING_LEN_96);
			if(PsStatus != XST_SUCCESS) {
				goto ERROR;
			}
			/* Assign the PPK0 sha3 hash */
			XilSKey_Efuse_ConvertStringToHexBE(
				(char *)XSK_EFUSEPS_PPK0_HASH,
				&PsInstancePtr->Ppk0Hash[0],
				XSK_EFUSEPS_PPK_SHA3HASH_LEN_IN_BITS_384);
		}
		/* If Sha2 hash is programming into Efuse PPK0 */
		else {
			/* Validation of PPK0 sha2 hash */
			PsStatus = XilSKey_Efuse_ValidateKey(
				(char *)XSK_EFUSEPS_PPK0_HASH,
				XSK_EFUSEPS_PPK_SHA2_HASH_STRING_LEN_64);
			if(PsStatus != XST_SUCCESS) {
				goto ERROR;
			}
			/* Assign the PPK0 sha3 hash */
			XilSKey_Efuse_ConvertStringToHexBE(
				(char *)XSK_EFUSEPS_PPK0_HASH,
				&PsInstancePtr->Ppk0Hash[0],
				XSK_EFUSEPS_PPK_SHA2HASH_LEN_IN_BITS_256);
		}
	}

	/* Is PPK1 hash programming is enabled */
	if (PsInstancePtr->PrgrmPpk1Hash == TRUE) {
		/* If Sha3 hash is programming into Efuse PPK1 */
		if (PsInstancePtr->IsPpk1Sha3Hash == TRUE) {
			/* Validation of PPK1 sha3 hash */
			PsStatus = XilSKey_Efuse_ValidateKey(
				(char *)XSK_EFUSEPS_PPK1_HASH,
				XSK_EFUSEPS_PPK_SHA3_HASH_STRING_LEN_96);
			if(PsStatus != XST_SUCCESS) {
				goto ERROR;
			}
			/* Assign the PPK1 sha3 hash */
			XilSKey_Efuse_ConvertStringToHexBE(
				(char *)XSK_EFUSEPS_PPK1_HASH,
				&PsInstancePtr->Ppk1Hash[0],
				XSK_EFUSEPS_PPK_SHA3HASH_LEN_IN_BITS_384);
		}
		/* If Sha2 hash is programming into Efuse PPK1 */
		else {
			/* Validation of PPK1 sha2 hash */
			PsStatus = XilSKey_Efuse_ValidateKey(
				(char *)XSK_EFUSEPS_PPK1_HASH,
				XSK_EFUSEPS_PPK_SHA2_HASH_STRING_LEN_64);
			if(PsStatus != XST_SUCCESS) {
				goto ERROR;
			}
			/* Assign the PPK1 sha2 hash */
			XilSKey_Efuse_ConvertStringToHexBE(
				(char *)XSK_EFUSEPS_PPK1_HASH,
				&PsInstancePtr->Ppk1Hash[0],
				XSK_EFUSEPS_PPK_SHA2HASH_LEN_IN_BITS_256);
		}
	}

	if (PsInstancePtr->PrgrmJtagUserCode == TRUE) {
		/* Validation of JTAG user code */
		PsStatus = XilSKey_Efuse_ValidateKey(
				(char *)XSK_EFUSEPS_JTAG_USERCODE,
				XSK_EFUSEPS_JTAG_USER_CODE_STRING_LEN);
		if (PsStatus != XST_SUCCESS) {
			goto ERROR;
		}
		/* Assign the JTAG user code */
		XilSKey_Efuse_ConvertStringToHexLE(
			(char *)XSK_EFUSEPS_JTAG_USERCODE,
			&PsInstancePtr->JtagUserCode[0],
			XSK_EFUSEPS_JTAG_USER_CODE_LEN_IN_BITS);
	}

	if (PsInstancePtr->PrgrmSpkID == TRUE) {
		/* Validation of SPK ID */
		PsStatus = XilSKey_Efuse_ValidateKey(
				(char *)XSK_EFUSEPS_SPK_ID,
				XSK_EFUSEPS_SPK_ID_STRING_LEN);
		if (PsStatus != XST_SUCCESS) {
			goto ERROR;
		}
		/* Assign the JTAG user code */
		XilSKey_Efuse_ConvertStringToHexLE(
			(char *)XSK_EFUSEPS_SPK_ID,
			&PsInstancePtr->SpkId[0],
			XSK_EFUSEPS_SPKID_LEN_IN_BITS);
	}



ERROR:
	return PsStatus;

}

/****************************************************************************/
/**
* This API reads secure control bits from efuse and prints the status bits
*
*
* @param	None
*
* @return
*		- XST_SUCCESS - In case of Success
*		- ErrorCode - If fails
*
* @note
*
*****************************************************************************/
static inline u32 XilSKey_EfusePs_Example_ReadSecCtrlBits()
{
	u32 PsStatus;
	XilSKey_SecCtrlBits ReadSecCtrlBits;

	PsStatus = XilSKey_ZynqMp_EfusePs_ReadSecCtrlBits(&ReadSecCtrlBits,
								XSK_EFUSEPS_RD_FROM_CACHE);
	if (PsStatus != XST_SUCCESS) {
		return PsStatus;
	}

	xil_printf("\r\nSecure and Control bits of eFuse:\n\r");

	if (ReadSecCtrlBits.AesKeyRead == TRUE) {
		xil_printf("\r\nAES key CRC check is disabled\n\r");
	}
	else {
		xil_printf("\r\nAES key CRC check is enabled\n\r");
	}

	if (ReadSecCtrlBits.AesKeyWrite == TRUE) {
		xil_printf("Programming AES key is disabled\n\r");
	}
	else {
		xil_printf("Programming AES key is enabled\n\r");
	}
	if (ReadSecCtrlBits.UseAESOnly == TRUE) {
		xil_printf("All boots must be encrypted with eFuse"
					"AES key is enabled\n\r");
	}
	else {
		xil_printf("All boots must be encrypted with eFuse"
					"AES key is disabled\n\r");
	}
	if (ReadSecCtrlBits.BbramDisable == TRUE) {
		xil_printf("Disables BBRAM key\n\r");
	}
	else {
		xil_printf("BBRAM key is not disabled\n\r");
	}
	if (ReadSecCtrlBits.PMUError == TRUE) {
		xil_printf("Error output from PMU is disabled\n\r");
	}
	else {
		xil_printf("Error output from PMU is enabled\n\r");
	}
	if (ReadSecCtrlBits.JtagDisable == TRUE) {
		xil_printf("Jtag is disabled\n\r");
	}
	else {
		xil_printf("Jtag is enabled\n\r");
	}
	if (ReadSecCtrlBits.DFTDisable == TRUE) {
		xil_printf("DFT is disabled\n\r");
	}
	else {
		xil_printf("DFT is enabled\n\r");
	}
	if (ReadSecCtrlBits.ProgGate0 == TRUE) {
		xil_printf("PROG_GATE 0 feature is disabled\n\r");
	}
	else {
		xil_printf("PROG_GATE 0 feature is enabled\n\r");
	}
	if (ReadSecCtrlBits.ProgGate1 == TRUE) {
		xil_printf("PROG_GATE 1 feature is disabled\n\r");
	}
	else {
		xil_printf("PROG_GATE 1 feature is enabled\n\r");
	}
	if (ReadSecCtrlBits.ProgGate2 == TRUE) {
		xil_printf("PROG_GATE 2 feature is disabled\n\r");
	}
	else {
		xil_printf("PROG_GATE 2 feature is enabled\n\r");
	}
	if (ReadSecCtrlBits.SecureLock == TRUE) {
		xil_printf("Reboot from JTAG mode is disabled when"
				"doing secure lock down\n\r");
	}
	else {
		xil_printf("Reboot from JTAG mode is enabled\n\r");
	}
	if (ReadSecCtrlBits.RSAEnable == TRUE) {
		xil_printf("RSA authentication is enabled\n\r");
	}
	else {
		xil_printf("RSA authentication is disabled\n\r");
	}
	if (ReadSecCtrlBits.PPK0WrLock == TRUE) {
		xil_printf("Locks writing to PPK0 efuse \n\r");
	}
	else {
		xil_printf("writing to PPK0 efuse is not locked\n\r");
	}

	if (ReadSecCtrlBits.PPK0Revoke == TRUE) {
		xil_printf("Revoking PPK0 is enabled \n\r");
	}
	else {
		xil_printf("Revoking PPK0 is disabled\n\r");
	}

	if (ReadSecCtrlBits.PPK1WrLock == TRUE) {
		xil_printf("Locks writing to PPK1 efuses\n\r");
	}
	else {
		xil_printf("writing to PPK1 efuses is not locked\n\r");
	}

	if (ReadSecCtrlBits.PPK1Revoke == TRUE) {
		xil_printf("Revoking PPK1 is enabled \n\r");
	}
	else {
		xil_printf("Revoking PPK1 is disabled\n\r");
	}

	xil_printf("\r\nUser control bits of eFuse:\n\r");

	if (ReadSecCtrlBits.UserWrLk0 == TRUE) {
		xil_printf("Programming USER_0 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_0 fuses is enabled\n\r");
	}
	if (ReadSecCtrlBits.UserWrLk1 == TRUE) {
		xil_printf("Programming USER_1 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_1 fuses is enabled\n\r");
	}
	if (ReadSecCtrlBits.UserWrLk2 == TRUE) {
		xil_printf("Programming USER_2 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_2 fuses is enabled\n\r");
	}
	if (ReadSecCtrlBits.UserWrLk3 == TRUE) {
		xil_printf("Programming USER_3 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_3 fuses is enabled\n\r");
	}
	if (ReadSecCtrlBits.UserWrLk4 == TRUE) {
		xil_printf("Programming USER_4 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_4 fuses is enabled\n\r");
	}
	if (ReadSecCtrlBits.UserWrLk5 == TRUE) {
		xil_printf("Programming USER_5 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_5 fuses is enabled\n\r");
	}
	if (ReadSecCtrlBits.UserWrLk6 == TRUE) {
		xil_printf("Programming USER_6 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_6 fuses is enabled\n\r");
	}
	if (ReadSecCtrlBits.UserWrLk7 == TRUE) {
		xil_printf("Programming USER_7 fuses is disabled\n\r");
	}
	else {
		xil_printf("Programming USER_7 fuses is enabled\n\r");
	}

	return XST_SUCCESS;

}