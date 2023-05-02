/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
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

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h" // Parameter definitions for processor periperals
#include "xscugic.h"     // Processor interrupt controller device driver
#include "xil_cache.h"
#include "xfpga417_fir_hw.h"
#include "xfpga417_fir.h"

//Instance Varible to Keep Reference to the HW
XFpga417_fir XFpga417_fir_inst;

void system_init();
void system_deinit();

int fpga417_init(XFpga417_fir *xFpga417_fir);
void fpga417_start(void *InstancePtr);

int run_hardware();

//Keep these small till further notice -- odd bug with the arm core...
#define INPUT_LENGTH 25
#define KERNEL_SIZE 25

int main()
{


	system_init();
	//Initialize the IP Core
	int status;
    //Setup the matrix mult
    status = fpga417_init(&XFpga417_fir_inst);
	if(status != XST_SUCCESS){
	  print("HLS peripheral setup failed\n\r");
	  exit(-1);
	}

	// 1. Define an array of real and imaginary KERNEL coefficients as specified in lab handout for testing FIR filter.
//	static int kernel_real[KERNEL_SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
//	static int kernel_img[KERNEL_SIZE] = {25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

	int kernel_real[KERNEL_SIZE] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	int kernel_img[KERNEL_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	// 2. Define sample inputs.
	int input_real[INPUT_LENGTH] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	int input_img[INPUT_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	for (int i = 0; i < KERNEL_SIZE; i++) {
		kernel_real[i] = 5;
		kernel_img[i] = -5;
	}

	for (int i = 0; i < INPUT_LENGTH; i++) {
		input_real[i] = 50-i;
		input_img[i] = i;
	}

	// Kernel buffers
	u32* KERNEL_REAL_BUFFER = XPAR_PS7_DDR_0_S_AXI_BASEADDR;
	u32* KERNEL_IMAG_BUFFER = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x20000;
	// Input buffers.
	u32* INPUT_REAL_BUFFER = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x40000;
	u32* INPUT_IMAG_BUFFER = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x60000;
	// Output buffers.
	u32* OUTPUT_MAG_BUFFER = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x80000;
	u32* OUTPUT_ANGLE_BUFFER = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0xa0000;

	// CLear out buffers
	memset(KERNEL_REAL_BUFFER, 0,KERNEL_SIZE<<2);
	memset(KERNEL_IMAG_BUFFER, 0,KERNEL_SIZE<<2);
	memset(INPUT_REAL_BUFFER, 0,INPUT_LENGTH<<2);
	memset(INPUT_IMAG_BUFFER, 0,INPUT_LENGTH<<2);
	memset(OUTPUT_MAG_BUFFER, 0,INPUT_LENGTH<<2);
	memset(OUTPUT_ANGLE_BUFFER, 0,INPUT_LENGTH<<2);

	// Initialize kernels.
	for (int i=0; i < KERNEL_SIZE; i++) {
		KERNEL_REAL_BUFFER[i] = kernel_real[i];
		KERNEL_IMAG_BUFFER[i] = kernel_img[i];
	}
	// Initialize input buffers.
	for (int i=0; i<INPUT_LENGTH; i++) {
		INPUT_REAL_BUFFER[i] = input_real[i];
		INPUT_IMAG_BUFFER[i] = input_img[i];
	}

	// Tell the FPGA the memory addresses in system memory where the arrays are at.
	XFpga417_fir_Set_input_real(&XFpga417_fir_inst, INPUT_REAL_BUFFER);
	XFpga417_fir_Set_input_img(&XFpga417_fir_inst, INPUT_IMAG_BUFFER);
	XFpga417_fir_Set_kernel_real(&XFpga417_fir_inst, KERNEL_REAL_BUFFER);
	XFpga417_fir_Set_kernel_img(&XFpga417_fir_inst, KERNEL_IMAG_BUFFER);
	XFpga417_fir_Set_output_mag(&XFpga417_fir_inst, OUTPUT_MAG_BUFFER);
	XFpga417_fir_Set_output_angle(&XFpga417_fir_inst, OUTPUT_ANGLE_BUFFER);
	// Set input length
	XFpga417_fir_Set_input_length(&XFpga417_fir_inst, INPUT_LENGTH);

	int err = run_hardware(); //when this function returns the result is ready
	//TODO: Compare results to a similar software functinon, print out the array etc...
	if (err) {
		print("Error");
	}

	// Print results.
	for (int i = 0; i < INPUT_LENGTH; i++) {
		printf("Output %d: Magnitude: %f, Phase: %f\n", i, *((float*)OUTPUT_MAG_BUFFER+i), *((float*)OUTPUT_ANGLE_BUFFER+i));
	}

	system_deinit();
    return 0;
}
void system_deinit()
{
	printf("Cleaning up...\n");
    cleanup_platform();
}
void system_init()
{
    printf("Initializing Zynq Processing System\n");
	init_platform();
	Xil_DCacheEnable();
	srand(0);
	printf("System Initialization Complete..\n");
}
int run_hardware()
{
	Xil_DCacheFlush();
	if (XFpga417_fir_IsReady(&XFpga417_fir_inst))
	  print("HLS peripheral is ready.  Starting... ");
	else {
	  print("!!! HLS peripheral is not ready! Exiting...\n\r");
	  return 1;
	}
	XFpga417_fir_Start(&XFpga417_fir_inst);
	int c = 0;
	while(!XFpga417_fir_IsReady(&XFpga417_fir_inst)){
	   printf("waiting for complete...%i\r", ++c);
	}
	Xil_DCacheInvalidate();
	return 0;
}
int fpga417_init(XFpga417_fir *fpga_417_ptr)
{
	XFpga417_fir_Config *cfgPtr;
	int status;
	printf("Initializing Accelerator\n");
	cfgPtr = XFpga417_fir_LookupConfig(XPAR_FPGA417_FIR_0_DEVICE_ID);
	if (!cfgPtr) {
	  fprintf(stderr, "ERROR: Lookup of acclerator configuration failed.\n\r");
	  return XST_FAILURE;
	}
	print("SUCCESS: Lookup of acclerator configuration succeeded.\n\r");
	status = XFpga417_fir_CfgInitialize(fpga_417_ptr, cfgPtr);
	if (status != XST_SUCCESS) {
	  print("ERROR: Could not initialize accelerator.\n\r");
	  return XST_FAILURE;
	}
	print("SUCCESS: initialized accelerator.\n\r");
	return status;
}
void fpga417_start(void *InstancePtr)
{
	XFpga417_fir *pAccelerator = (XFpga417_fir *)InstancePtr;
	XFpga417_fir_InterruptEnable(pAccelerator,1);
	XFpga417_fir_InterruptGlobalEnable(pAccelerator);
	XFpga417_fir_Start(pAccelerator);
}
