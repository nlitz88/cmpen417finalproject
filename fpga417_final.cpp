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
void fir(int input_r, int input_i, int filter_r[KERNEL_SIZE], int filter_i[KERNEL_SIZE], int* output_r, int* output_i) {

	// Statically define the
	// Create a static shift register that we'll push each received input into. Will only be the
	// length of the filter, though, as that's all we need for the element-wise multiplication.
	// Might not be able to use this define here to allocate this.
	static int inputs_r_shiftreg[KERNEL_SIZE] = {0}; 	// THIS HAS TO BE INITIALIZED TO 0!
	static int inputs_i_shiftreg[KERNEL_SIZE] = {0};		// THIS HAS TO BE INITIALIZED TO 0!
	// Variables to accumulate each of the products. I.e., stores the sum of products == the dot product.
	int dot_product_r = 0;
	int dot_product_i = 0;
	// Variables to store the real and imaginary result from complex multiplication function.
	int real_result = 0;
	int imag_result = 0;
	// Iterator variable.
	int i;

	// Loop to compute the dot product. Start from the right side of shift register/filter.
	for(i = KERNEL_SIZE - 1; i >= 0; i--) {

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
	LOOP_FIR_MAIN: for (i = 0; i < length; i++) {
		// Pass the ith input value into the fir filter (really just taking the dot product).
		fir(input_real[i], input_img[i], kernel_real, kernel_img, &iteration_r_result, &iteration_i_result);
		// Use the blocking stream api function "write" to push each value to its respective stream.
		output_real.write(iteration_r_result);
		output_img.write(iteration_i_result);
	}

	return;
}

// Function to perform each CORDIC rotation (conversion from cartesian to polar).
void cordic(int cos, int sin, float *output_mag, float *output_angle) {

	// Initialization.
	FIXED_POINT current_cos = (FIXED_POINT)cos;
	FIXED_POINT current_sin = (FIXED_POINT)sin;
	FIXED_POINT theta_rotated = 0.0;
	// Some constants for the initial angle offset.
	FIXED_POINT ninety = 1.5708;
	FIXED_POINT half_circle = 3.14159;
	FIXED_POINT three_quart = 4.71239;
	FIXED_POINT cricle = 6.28319;
	FIXED_POINT angle_offset = 0; // NOTE: I think we can get rid of this variable and just add the offset to the theta_directly
	// at the beginning--as we're just going to add it to the end anyways!

	// Logic here to convert the coordinates based on what quadrant they fall in.

	// If the provided coordinates fall in the second quadrant.
	if (current_cos < 0 && current_sin > 0) {
		angle_offset = ninety;
		current_cos = -current_cos;
	}
	// If the provided coordinates fall in the third quadrant.
	else if (current_cos < 0 && current_sin < 0) {
		angle_offset = half_circle;
		current_cos = -current_cos;
		current_sin = -current_sin;
	}
	// If the provided coordinates fall in the fourth quadrant.
	else if (current_cos > 0 && current_sin < 0) {
		angle_offset = three_quart;
		current_sin = -current_sin;
	}
	else {
		angle_offset = 0;
	}

	// Now, this is where we'll iterate through and perform the rotation.
	for (int j = 0; j < NUM_CORDIC_ITERATIONS; j++) {
	      // Multiply previous iteration by 2^(-j).  This is equivalent to
	      // a right shift by j on a fixed-point number.
	      FIXED_POINT cos_shift = current_cos >> j;
	      FIXED_POINT sin_shift = current_sin >> j;


	    // FOR THE FIRST QUADRANT:
	    // A positive rotation (counter-clockwise) corresponds with the cosine (x) value decreasing and the sine (y) value increasing.

	    // Our goal: rotate the vector to get it as close to 0 degrees as possible (meaning a sin value as close to zero as possible).

	    // If the current y-value is above the x axis, then rotate CLOCKWISE to get closer to the x-axis
	    if(current_sin >= 0) {

	    	// Clockwise rotation == increase in cosine (x) value, decrease in sine (y) value.
	        current_cos = current_cos + sin_shift;
	        current_sin = current_sin - cos_shift;

	        // For this rotation, should it be added or subtracted from the rotated theta?

	        // So, for the rotated theta, this is tracking the number of radians we've rotated FROM our original coordinates--right?
	        // Therefore, if our coordinates start somewhere in the first quadrant, and we rotate clockwise, then shouldn't that rotation
	        // be counted as "positive" degrees towards our rotation? Right?

	        // Because, ultimately, the angle the coordinates are at are how many degrees/radians we rotate from the point BACK to the
	        // x-axis! Therefore,
	        theta_rotated = theta_rotated + cordic_phase[j];

	    }
	    else {
	        // Counter-clockwise rotation == decrease in cosine (x) value, decrease in sine (y) value.
	        current_cos = current_cos - sin_shift;
	        current_sin = current_sin + cos_shift;

	        // Though the rotation itself is counter-clockwise (which is positive), this rotation would decrease the number of degrees
	        // we've rotated FROM the coordinates to the x-axis--therefore we subtract the rotation angle here.
	        theta_rotated = theta_rotated - cordic_phase[j];
	    }
	}

	// Now that we've performed the NUM_CORDIC_ITERATIONS rotations, we can come up with our final values.

	// The magnitude is computed as the product of the cordic gain (after NUM_CORDIC_ITERATIONS) times the current_cos value.
	*output_mag = ((FIXED_POINT)(current_cos*CORDIC_GAIN)).to_float();

	// Shouldn't have to do anything to the theta_rotated, as the offset would have already been contributed from the start.
	*output_angle = theta_rotated.to_float();

	return;

}

// Prototype for CORDIC rotator used to convert cartesian outputs of top_fir to polar form.
void top_cordic_rotator(hls::stream<int>&input_real, hls::stream<int>&input_img, float* output_mag, float* output_angle, int length) {

	// Variables to store values popped from the queue that the FIR filter feeds.
	int temp_result_real = 0;
	int temp_result_img = 0;
	float temp_result_mag = 0;		// These variables may not be strictly necessary, if we provide the array address to the cordic function.
	float temp_result_angle = 0;

	int i;
	// Main cordic rotator loop. Read "length" values from the output stream, as FIR filter only outputs length values.
	LOOP_CORDIC_MAIN: for (i = 0; i < length; i++) {

		// Read the real and imaginary outputs from the respective streams the FIR filter writes to.
		temp_result_real = input_real.read();
		temp_result_img = input_img.read();

		// Call the cordic function to convert the complex number in cartesian form to polar form.
		cordic(temp_result_real, temp_result_img, &temp_result_mag, &temp_result_angle);

		// Write the resulting magnitude and angle to their respective output arrays.
		output_mag[i] = temp_result_mag;
		output_angle[i] = temp_result_angle;

	}

	return;
}


// Prototype for overarching hardware FIR filter that takes in complex inputs and produces convolution
// result values in polar form. Does this by connecting top_fir and top_cordic_rotator.
void fpga417_fir(int* input_real, int* input_img, int* kernel_real, int* kernel_img, float* output_mag, float* output_angle, int input_length) {

#pragma HLS INTERFACE port=return mode=s_axilite
	// Use m_axi for all array address arguments, as we want to access these arrays in the processing system's
	// memory via a DMA controller connected to an AXI full bus.
#pragma HLS INTERFACE port=input_real mode=m_axi bundle=BUS_A
#pragma HLS INTERFACE port=input_img mode=m_axi bundle=BUS_A
#pragma HLS INTERFACE port=output_mag mode=m_axi bundle=BUS_A
#pragma HLS INTERFACE port=output_angle mode=m_axi bundle=BUS_A
	// Module can't read both the kernel coefficients and inputs from the same AXI bus--so we place the
	// kernel ports on a separate AXI bus/interface.
#pragma HLS INTERFACE port=kernel_real mode=m_axi bundle=BUS_B
#pragma HLS INTERFACE port=kernel_img mode=m_axi bundle=BUS_B
	// Use s_axilite for scalar value argument, as it can be accessed as a single register value.
#pragma HLS INTERFACE port=input_length mode=s_axilite

	// Question: Dataflow pragma only needed at this top level, so as to indicate to Vitis that the functions we call
	// under this pragma are to implement a DATAFLOW via hls::streams present between the functions.
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
