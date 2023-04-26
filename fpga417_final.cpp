#include <stdio.h>
#include "fpga417_final.h"

// Create function that performs complex multiplication.
void complex_mult(int ar, int ai, int br, int bi, int* o_r, int* o_i) {

	// As a hardware instance, we want this to use four DSPs.
	// Is there anything special we have to do to enable this?
	*o_r = ar*br - ai*bi;
	*o_i = ar*bi + ai*br;
	// Also have to take into account the width of the operands we're doing the multiplication with.
	// I.e., multipliers only have so much width, so with a 32 bit integer, would have to split up a
	// single multiplication into two multiplier DSPs.
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

// "Top" fir function that takes in array of complex inputs and produces the convolution of those complex inputs
// with an internally defined filter. Result is returned as two parallel arrays; one contains the real values
// of the results and one contains the imaginary.
void top_fir(int* input_real, int* input_img, int kernel_real[KERNEL_SIZE], int kernel_img[KERNEL_SIZE], hls::stream<int>&output_real, hls::stream<int>&output_img, int length) {

	// Need to define interface for each port.

	// No need to store the kernel values locally, as they are already instantiated as a block ram on the FPGA within the fpga417_fir module.
	// Additionally, going to experiment with not yet storing the input values locally. Not sure if that's a better or worse
	// idea.

	// Create a variable to store the output at each sliding window position-->ultimately written to the output stream.
	int iteration_r_result;
	int iteration_i_result;

	// Perform (partial) convolution with input data and filter coefficients. Write the output of each
	// convolution step (dot product) to the input that was just fed in to produce that next output!
	// This means we're not computing a complete convolution--so it's not technically completely correct.
	// But we're more so interested in doing this just for demonstration.
	int i;
	for (i = 0; i < length; i++) {
		// Pass the ith input value into the fir filter (really just taking the dot product).
		fir(input_real[i], input_img[i], kernel_real, kernel_img, &iteration_r_result, &iteration_i_result);
		// Use the blocking stream api function "write" to push each value to its respective stream.
		output_real.write(iteration_r_result);
		output_img.write(iteration_i_result);
	}

	return;
}

// Prototype for CORDIC rotator used to convert cartesian outputs of top_fir to polar form.
void top_cordic_rotator(hls::stream<int>&input_real, hls::stream<int>&input_img, float* output_mag, float* output_angle, int length) {

	// Need to define interfaces for each input.
	return;
}


// Prototype for overarching hardware FIR filter that takes in complex inputs and produces convolution
// result values in polar form. Does this by connecting top_fir and top_cordic_rotator.
void fpga417_fir(int* input_real, int* input_img, int* kernel_real, int* kernel_img, float* output_mag, float* output_angle, int input_length) {

#pragma HLS DATAFLOW

	// Load the kernel values onto FPGA.
	int filter_real[KERNEL_SIZE];
	int filter_img[KERNEL_SIZE];
	int i;
	for (i = 0; i < KERNEL_SIZE; i++) {
		filter_real[i] = kernel_real[i];
		filter_img[i] = kernel_img[i];
	}

	// Initialize the FIFO buffers that will store the values that go between the FIR filter and the CORDIC rotator.
	hls::stream<int> real_stream;
	hls::stream<int> img_stream;

	// Call the top_fir filter on the inputs. This will write its outputs in cartesian form to the real and imaginary FIFO streams.
	top_fir(input_real, input_img, filter_real, filter_img, real_stream, img_stream, input_length);
	// Then call the CORDIC rotator on the cartesian outputs of the FIR filter to convert the output values to polar form.
	top_cordic_rotator(real_stream, img_stream, output_mag, output_angle, input_length);


	return;
}


void fpga417_fir(int input_r[DATA_LENGTH], int input_i[DATA_LENGTH], int output_r[DATA_LENGTH], int output_i[DATA_LENGTH]) {

#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE m_axi port=input offset=slave // bundle=gmem0
#pragma HLS INTERFACE m_axi port=filter offset=slave // bundle=gmem1...

	// We don't specify depth--maybe we need that?
	// We also don't specify a bundle for any of our interfaces--this is because we actually don't
	// want multiple interfaces to be bundled/connected together to the same AXI interconnect.
	// By default, they seem to be put in separate bundles.


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
