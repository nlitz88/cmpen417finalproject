#include <stdio.h>

#include "fpga417_final.h"

// Create function that performs complex multiplication.
void complex_mult(int ar, int ai, int br, int bi, int* or, int* oi) {

	// As a hardware instance, we want this to use four DSPs.
	// Is there anything special we have to do to enable this?
	*or = ar*br - ai*bi;
	*oi = ar*bi + ai*br;
	return;
}

// Function that performs a single step of convolution == a dot product with the kernel
// and the current values of the input within the sliding window.
void fir(int input_r, int input_i, int filter_r[FILTER_LENGTH], int filter_i[FILTER_LENGTH], int* output_r, int* output_i) {

	// Statically define the
	// Create a static shift register that we'll push each received input into. Will only be the
	// length of the filter, though, as that's all we need for the element-wise multiplication.
	// Might not be able to use this define here to allocate this.
	static int inputs_r_shiftreg[FILTER_LENGTH] = { [0 ... FILTER_LENGTH - 1] = 0}; 	// THIS HAS TO BE INITIALIZED TO 0!
	static int inputs_i_shiftreg[FILTER_LENGTH] = { [0 ... FILTER_LENGTH - 1] = 0};		// THIS HAS TO BE INITIALIZED TO 0!
	// Variables to accumulate each of the products. I.e., stores the sum of products == the dot product.
	int dot_product_r = 0;
	int dot_product_i = 0;
	// Variables to store the real and imaginary result from complex multiplication function.
	int real_result = 0;
	int imag_result = 0;
	// Iterator variable.
	int i;

	// Loop to compute the dot product. Start from the right side of shift register/filter.
	for(i = FILTER_LENGTH - 1; i >= 0; i--) {

		// If on the first multiplication, set the leading element of the shift register to the input.
		if(i == 0) {
			inputs_r_shiftreg[0] = input_r;
			inputs_i_shiftreg[0] = input_i;
		}

		// Otherwise, set the current value of the shift register to the value that comes before it
		// (performing the shift), and then compute the multiplication between the new element at that
		// position and the corresponding filter value.
		else {
			inputs_r_shiftreg[i] = inputs_r_shiftreg[i-1];
			inputs_i_shiftreg[i] = inputs_i_shiftreg[i-1];
		}

		// Regardless of which number we're on, we'll compute the current position's dot product
		// using the complex multiplier.
		complex_mult(inputs_r_shiftreg[i], inputs_i_shiftreg[i], filter_r[i], filter_i[i], &real_result, &imag_result);

		// Add the real result and imaginary result to the real and imaginary dot product, respectively.
		dot_product_r += real_result;
		dot_product_i += imag_result;
	}

	// Finally, write the computed dot product (this single step of the overall convolution) to the output.

	*output_r = dot_product_r;
	*output_i = dot_product_i;

	return;
}

void fpga417_fir(int* input, int* filter) {

#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE m_axi port=input offset=slave // bundle=gmem0
#pragma HLS INTERFACE m_axi port=filter offset=slave // bundle=gmem1...

	// We don't specify depth--maybe we need that?
	// We also don't specify a bundle for any of our interfaces--this is because we actually don't
	// want multiple interfaces to be bundled/connected together to the same AXI interconnect.
	// By default, they seem to be put in separate bundles.

	static const int filter_real[FILTER_LENGTH] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
	static const int filter_imag[FILTER_LENGTH] = {25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};
	int temp;
	int i;

	// Load in filter coefficients. Because filter_coeff is automatically allocated,
	// I think this'll correspond to allocating block RAM on the FPGA. Therefore, want to load
	// the data from DRAM to BRAM before performing convolution, so as to not perform a data read
	// every call to fir.
	for (i = 0; i < FILTER_LENGTH; i++) {
		filter_coeff[i] = filter[i];
	}

	// Perform (partial) convolution with input data and filter coefficients. Write the output of each
	// convolution step (dot product) to the input that was just fed in to produce that next output!
	// This means we're not computing a complete convolution--so it's not technically completely correct.
	// But we're more so interested in doing this just for demonstration.
	for (i = 0; i < DATA_LENGTH; i++) {
		// Pass the ith input value into the fir filter (really just taking the dot product).
		fir(input[i], filter_coeff, &temp);
		input[i] = temp;
	}

}
