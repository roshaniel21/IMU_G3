/*
 * vector3.c
 *
 *  Created on: Nov 11, 2015
 *      Author: Daniel Greenheck
 */

void VectorAdd(float **result, float *a, float *b) {
	float retVal[3] = {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
	*result = retVal;
	return;
}

void VectorSub(float **result, float *a, float *b) {
	float retVal[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
	*result = retVal;
	return;
}


void VectorMult(float **result, float *a, float c) {
	float retVal[3] = {c*a[0], c*a[1], c*a[2]};
	*result = retVal;
	return;
}

void VectorCrossMult(float **result, float *a, float *b) {
	float retVal[3] = {0};
	retVal[0] = a[1]*b[2] - a[2]*b[1];
	retVal[1] = a[2]*b[0] - a[0]*b[2];
	retVal[2] = a[0]*b[1] - a[1]*b[0];
	*result = retVal;
	return;
}
