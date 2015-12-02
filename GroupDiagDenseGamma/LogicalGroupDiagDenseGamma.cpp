/*
 * Copyright (C) 2015 Yiqun Zhang <zhangyiqun9164@gmail.com>
 * All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This Gamma operator is proposed in the paper:
 * The Gamma Operator for Big Data Summarization on an Array DBMS
 * Carlos Ordonez, Yiqun Zhang, Wellington Cabrera 
 * Journal of Machine Learning Research (JMLR): Workshop and Conference Proceedings (BigMine 2014) 
 *
 * Please cite the paper above if you need to use this code in your research work.
 */

/*
 * @file LogicalGroupDiagDenseGamma.cpp
 * @author Yiqun Zhang <zhangyiqun9164@gmail.com>
 *
 * @brief Logical part of the GroupDiagDenseGamma SciDB operator.
 */

#include <query/Operator.h>
using namespace std;

namespace scidb {

class LogicalGroupDiagDenseGamma : public LogicalOperator {
public:

  LogicalGroupDiagDenseGamma(const string& logicalName, const string& alias):
    LogicalOperator(logicalName, alias) {
    /* 
     * For K-means and other EM model, we call GroupDiagDenseGamma( X, XNN ); 
     * For Naive Bayesian Classifier, we can call GroupDiagDenseGamma( X, filter(X,j=d+1) ); 
     */ 
    ADD_PARAM_INPUT()
    ADD_PARAM_INPUT()
    ADD_PARAM_CONSTANT(TID_INT64) // Goup number k.
  }

  ArrayDesc inferSchema(vector< ArrayDesc> schemas, shared_ptr< Query> query) {
    ArrayDesc const& inputSchema = schemas[0]; // X( [i, j], v )
    ArrayDesc const& groupSchema = schemas[1]; // XNN( [i], g ), for NB classifier, it may be XNN( [i, j=d+1], Y )
    Dimensions inputDims = inputSchema.getDimensions();
    Dimensions groupDims = groupSchema.getDimensions();
    // The input array should have 2 dimensions: i and j.
    if(inputDims.size() != 2) {
      throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_ILLEGAL_OPERATION)
          << "Input data set should be 2-dimensional.";
    }
    Coordinate k = evaluate(((boost::shared_ptr<OperatorParamLogicalExpression>&)_parameters[0])->getExpression(),
                        query, TID_INT64).getInt64();
    if(k < 1) {
      throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_ILLEGAL_OPERATION)
          << "k has to be greater or equal to 1.";
    }
    Attributes inputAttrs = inputSchema.getAttributes(true);
    Attributes classAttrs = groupSchema.getAttributes(true);
    // TODO: This restriction can be lifted.
    // The input array should have only 1 attribute, and it should be in double type.
    if (inputAttrs.size() != 1 ||
      inputAttrs[0].getType() != TID_DOUBLE) { // It can be int64...
      throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_ILLEGAL_OPERATION)
          << "Attribute of input array should be double.";
    }
    DimensionDesc inputDimsN = inputDims[0];
    DimensionDesc groupDimsN = groupDims[0];
    DimensionDesc dimsD = inputDims[1];
    // We assume the input data set has d columns and an additional Y columns.
    Coordinate inputN = inputDimsN.getCurrLength();
    Coordinate groupN = groupDimsN.getCurrLength();
    Coordinate d = dimsD.getCurrLength() - 1;
    if(dimsD.getChunkInterval() < d+1) {
      throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_ILLEGAL_OPERATION)
          << "Chunk size of the column dimension must be no less than d+1.";
    }
    if(inputN != groupN) {
      throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_ILLEGAL_OPERATION)
          << "Number of points in input array and group array should be the same.";
    }
    if( groupDimsN.getChunkInterval() != inputDimsN.getChunkInterval() ) {
      throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_ILLEGAL_OPERATION)
          << "Chunk size of the first dimension in input array and group array should be the same.";
    }
    if (classAttrs.size() != 1 ||
      classAttrs[0].getType() != TID_INT64) { 
      throw SYSTEM_EXCEPTION(SCIDB_SE_OPERATOR, SCIDB_LE_ILLEGAL_OPERATION)
          << "Attribute of class array should be int64.";
    }
    // The output array has only one attribute which is exactly the same as the input.
    Attributes outputAttributes;
    outputAttributes.push_back( inputSchema.getAttributes(true)[0] );
    outputAttributes = addEmptyTagAttribute(outputAttributes);
    Dimensions outputDims;
    // We set i start from 1.
    DimensionDesc i("i", 1, k, k, 0); // Class
    	DimensionDesc j("j", 1, 2*d+3, 2*d+3, 0);
    	outputDims.push_back(i); 
    	outputDims.push_back(j); 
    return ArrayDesc("GroupDiagDenseGamma_" + inputSchema.getName(), outputAttributes, outputDims, defaultPartitioning());
  }
};

REGISTER_LOGICAL_OPERATOR_FACTORY(LogicalGroupDiagDenseGamma, "GroupDiagDenseGamma");

} //namespace scidb