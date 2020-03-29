#pragma once

#include "WOGridECEFElevation.h"
#include <vector>


namespace Aftr {
	class WO;

	class WOPhysXTriangularMesh : public WOGridECEFElevation {
	public:
		static WOPhysXTriangularMesh * New(const VectorD& upperLeft, const VectorD& lowerRight, unsigned int granularity, const VectorD& offset, const VectorD& scale, std::string elevationData, int splits = 2, float exageration = 0, bool useColors = false);
				
	protected:
		WOPhysXTriangularMesh();
		virtual void onCreate(const VectorD & upperLeft, const VectorD & lowerRight, unsigned int granularity, const VectorD & offset, const VectorD & scale, std::string elevationData, int splits, float exageration, bool useColors);
			};
}