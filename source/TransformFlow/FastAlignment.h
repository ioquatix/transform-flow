//
//  Alignment.h
//  IntegerArrayAlignment
//
//  Created by Samuel Williams on 20/09/13.
//  Copyright (c) 2013 Orion Transfer. All rights reserved.
//

#ifndef __IntegerArrayAlignment__Alignment__
#define __IntegerArrayAlignment__Alignment__

#include "FeatureTable.h"

#include <Euclid/Numerics/Average.h>

namespace TransformFlow
{
	using namespace Euclid::Numerics;
	using namespace TransformFlow;

	Average<RealT> align_tables(const FeatureTable & a, const FeatureTable & b, int estimate = 0);
}

#endif /* defined(__IntegerArrayAlignment__Alignment__) */
