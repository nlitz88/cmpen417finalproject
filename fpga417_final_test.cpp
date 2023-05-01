#include <stdio.h>
#include <stdlib.h>

#include "fpga417_final.h"

int * test_filter;
int * hardware_input;
int * reference_input;

void init();
void deinit();
void initialize_array(int ** array, int length);
int compare_array(int * a, int * b, int length);
void copy_array(int * a, int * b, int length);
void print_array(int* a, int length);
void reference_fir(int* data, int* filter);


int main(void)
{

	// 1. Define an array of real and imaginary KERNEL coefficients as specified in lab handout for testing FIR filter.
	static const int coef_real[KERN]
	init();

	// Call reference FIR (convolution) function on initialized data.
	reference_fir(reference_input, test_filter);
	// Call hardawre FIR implementation on the same inputs.
	fpga417_fir(hardware_input, test_filter);

	// Print the result of each.
	printf("\nSoftware Generated Result:\n");
	print_array(reference_input, DATA_LENGTH);
	printf("\nHardware Generated Result:\n");
	print_array(hardware_input, DATA_LENGTH);
	printf("\n\n");

	// Compare the results.
	if(compare_array(reference_input, hardware_input, DATA_LENGTH) == 1) {
		printf("HW does not match SW\n");
	}
	else {
		printf("HW and SW match\n");
	}

	deinit();
	return 0;
}
void deinit()
{
	free(test_filter);
	free(hardware_input);
	free(reference_input);
}
void init()
{

	srand(0);
	initialize_array(&test_filter, FILTER_LENGTH);
	initialize_array(&hardware_input, DATA_LENGTH);
	initialize_array(&reference_input, DATA_LENGTH);
	// Copy the contents of the reference_input into the hardware input so that we feed them in the same
	// randomly initialized array data.
	copy_array(hardware_input, reference_input, DATA_LENGTH);

}

// NOTE: DATA_LENGTH of data and FILTER_LENGTH defined in included fpga417_lab4.h
void reference_fir(int* input_data, int* filter) {

	int dot_product;
	int input_shift_reg[FILTER_LENGTH] = { [0 ... FILTER_LENGTH - 1] = 0};
	int i;
	int j;


	for (i = 0; i < DATA_LENGTH; i++) {

		dot_product = 0;
		// Feed in each value of the input to the shift register, one at a time.
		// Shift values in register.
		for (j = FILTER_LENGTH - 1; j >= 0; j--) {

			if (j == 0) {
				dot_product += input_data[i]*filter[j];
				input_shift_reg[0] = input_data[i];
			}
			else {
				input_shift_reg[j] = input_shift_reg[j-1];
				dot_product += input_shift_reg[j]*filter[j];
			}

		}

		// Write dot product to output.
		input_data[i] = dot_product;

	}

}

void initialize_array(int ** arr, int length)
{
	int idx;
	*arr = (int *)malloc(sizeof(int) * length);
	for(idx = 0; idx<length; ++idx)
		(*arr)[idx] = rand() % 255;
}
int compare_array(int * a, int * b, int length)
{
	int i;
	for(i = 0; i< length; ++i) if(a[i] != b[i]) return 1;
	return 0;
}
void copy_array(int * a, int * b, int length)
{
	int i;
	for(i = 0; i< length; ++i) a[i] = b[i];
}
void print_array(int* a, int length) {
	int i;
	for (i = 0; i < length; i++) {
		printf("%d ", a[i]);
	}
}
