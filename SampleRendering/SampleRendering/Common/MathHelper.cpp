//***************************************************************************************
// MathHelper.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "MathHelper.h"
#include <float.h>
#include <cmath>

using namespace DirectX;

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi       = 3.1415926535f;


double MathHelper::IntegerRadicalInverse(int Base, int i)
{
	int numPoints, inverse;
	numPoints = 1;
	// 此循环将i在"Base"进制下的数字左右Flip
	for (inverse = 0; i > 0; i /= Base)
	{
		inverse = inverse * Base + (i % Base);
		numPoints = numPoints * Base;
	}

	// 除以Digit将这个数镜像到小数点右边
	return inverse / (double)numPoints;
}


double MathHelper::RadicalInverse(int Base, int i)
{

	double Digit, Radical, Inverse;
	Digit = Radical = 1.0 / (double)Base;
	Inverse = 0.0;
	while (i)
	{
		// i余Base求出i在"Base"进制下的最低位的数
		// 乘以Digit将这个数镜像到小数点右边
		Inverse += Digit * (double)(i % Base);
		Digit *= Radical;

		// i除以Base即可求右一位的数
		i /= Base;
	}
	return Inverse;

}

float MathHelper::AngleFromXY(float x, float y)
{
	float theta = 0.0f;
 
	// Quadrant I or IV
	if(x >= 0.0f) 
	{
		// If x = 0, then atanf(y/x) = +pi/2 if y > 0
		//                atanf(y/x) = -pi/2 if y < 0
		theta = atanf(y / x); // in [-pi/2, +pi/2]

		if(theta < 0.0f)
			theta += 2.0f*Pi; // in [0, 2*pi).
	}

	// Quadrant II or III
	else      
		theta = atanf(y/x) + Pi; // in [0, 2*pi).

	return theta;
}

XMVECTOR MathHelper::RandUnitVec3()
{
	XMVECTOR One  = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	// Keep trying until we get a point on/in the hemisphere.
	while(true)
	{
		// Generate random point in the cube [-1,1]^3.
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// Ignore points outside the unit sphere in order to get an even distribution 
		// over the unit sphere.  Otherwise points will clump more on the sphere near 
		// the corners of the cube.

		if( XMVector3Greater( XMVector3LengthSq(v), One) )
			continue;

		return XMVector3Normalize(v);
	}
}

XMVECTOR MathHelper::RandHemisphereUnitVec3(XMVECTOR n)
{
	XMVECTOR One  = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	// Keep trying until we get a point on/in the hemisphere.
	while(true)
	{
		// Generate random point in the cube [-1,1]^3.
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// Ignore points outside the unit sphere in order to get an even distribution 
		// over the unit sphere.  Otherwise points will clump more on the sphere near 
		// the corners of the cube.
		
		if( XMVector3Greater( XMVector3LengthSq(v), One) )
			continue;

		// Ignore points in the bottom hemisphere.
		if( XMVector3Less( XMVector3Dot(n, v), Zero ) )
			continue;

		return XMVector3Normalize(v);
	}
}