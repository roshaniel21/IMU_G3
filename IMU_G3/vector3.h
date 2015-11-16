/*
 * vector3.h
 *
 *  Created on: Nov 11, 2015
 *      Author: Daniel Greenheck
 */

#ifndef VECTOR3_H_
#define VECTOR3_H_

// The sum of vector 'a' and vector 'b' is returned in 'result'
void VectorAdd(float **result, float *a, float *b);

// The difference of vector 'a' and vector 'b' is returned in 'result'
void VectorSub(float **result, float *a, float *b);

// The product of vector 'a' and scalar constant 'c' is returned in 'result'
void VectorMult(float **result, float *a, float c);

// The cross product of 'a' and 'b' is returned in 'result'
void VectorCrossMult(float **result, float *a, float *b);


#endif /* VECTOR3_H_ */
