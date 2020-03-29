#include "WOPhysXTriangularMesh.h"
#include <iostream>

using namespace Aftr;


WOPhysXTriangularMesh* WOPhysXTriangularMesh::New(const VectorD& upperLeft, const VectorD& lowerRight, unsigned int granularity, const VectorD& offset, const VectorD& scale, std::string elevationData, int splits, float exageration, bool useColors) {
	WOPhysXTriangularMesh* grid = new WOPhysXTriangularMesh();
	grid->onCreate(upperLeft, lowerRight, granularity, offset, scale, elevationData, splits, exageration, useColors);
	return grid;
}

WOPhysXTriangularMesh::WOPhysXTriangularMesh() : IFace(this), WOGridECEFElevation() {
}

void WOPhysXTriangularMesh::onCreate(const VectorD& upperLeft, const VectorD& lowerRight, unsigned int granularity, const VectorD& offset, const VectorD& scale, std::string elevationData, int splits = 2, float exageration = 0, bool useColors = false) {
	WOGridECEFElevation::onCreate(upperLeft, lowerRight, 0, offset, scale, elevationData, splits, exageration, useColors);
}