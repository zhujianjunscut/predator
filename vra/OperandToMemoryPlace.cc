/**
* @author Daniela Ďuričeková, xduric00@stud.fit.vutbr.cz
* @file   OperandToMemoryPlace.cc
* @brief  Implementation of class that converts @c cl_operand to the instance
*         of the @c MemoryPlace class.
* @date   2012
*/

#undef NDEBUG

#include <string>
#include <cassert>
#include <iostream>
#include "OperandToMemoryPlace.h"

using std::string;
using std::vector;
using std::map;
using std::deque;

map<OperandToMemoryPlace::UidVector, MemoryPlace*>
	OperandToMemoryPlace::memoryPlaceMap;

/**
* @brief Converts @c cl_operand to the instance of the @c MemoryPlace class. Used only
*        for simple variables, elements of array, items of structures.
*
* @param[in] operand It will be converted to the @c MemoryPlace object.
*
* @return @c MemoryPlace instance that was created from @a operand.
*
* Preconditions:
* - @code operand->code == CL_OPERAND_VAR @endcode
*/
MemoryPlace* OperandToMemoryPlace::convertSimpleOperand(const cl_operand *operand)
{
	// Checks if the precondition is satisfied.
	assert(operand->code == CL_OPERAND_VAR);

	// Stores the name of the variable.
	string name;
	if (((operand->data).var)->name != NULL) {
		// This is necessary because of artificial variables.
		name = string(((operand->data).var)->name);
	}

	// Stores if the variable was generated by the compiler.
	bool artificial = ((operand->data).var)->artificial;
	if (name.empty()) {
		// In some cases, it happens that artificial variable is set as
		// not artificial and in the output, we have variables without name.
		artificial = true;
	}

	// Stores the unique id of the variable.
	Int uid = ((operand->data).var)->uid;

	// Vector represents unique id for the given operand.
	vector<Int> uidVector;
	uidVector.push_back(uid);

	if (NULL == operand->accessor) {
		// If the given cl_operand represents a simple variable.
		if (OperandToMemoryPlace::memoryPlaceMap.count(uidVector) == 0) {
			// This variable is used for the first time.
			MemoryPlace *var = new MemoryPlace(name, artificial);
			OperandToMemoryPlace::memoryPlaceMap[uidVector] = var;
			return var;
		} else {
			// This variable was used before. We return found record.
			return OperandToMemoryPlace::memoryPlaceMap[uidVector];
		}
	} else if (CL_ACCESSOR_ITEM == (operand->accessor)->code ||
			   CL_ACCESSOR_DEREF_ARRAY == (operand->accessor)->code) {
		// If the given cl_operand represents an item of a structure or
		// if the given cl_operand represents an element of an array.
		struct cl_accessor *actualAccessor = operand->accessor;
		while (actualAccessor != NULL) {
			if (CL_ACCESSOR_ITEM == actualAccessor->code) {
				// If the given cl_operand represents an item of a structure.
				int index = actualAccessor->data.item.id;
				uidVector.push_back(index);
				name += string(".");
				name += string((((actualAccessor->type)->items)[index]).name);
			} else if (CL_ACCESSOR_DEREF_ARRAY == actualAccessor->code) {
				// If the given cl_operand represents an element of an array.
				name += string("[]");
				// Element of an array has always only one item.
				uidVector.push_back(0);
			}

			actualAccessor = actualAccessor->next;
		}

		if (OperandToMemoryPlace::memoryPlaceMap.count(uidVector) == 0) {
			// This variable is used for the first time.
			MemoryPlace *var = new MemoryPlace(name, artificial);
			OperandToMemoryPlace::memoryPlaceMap[uidVector] = var;
			return var;
		} else {
			// This variable was used before. We return found record.
			return OperandToMemoryPlace::memoryPlaceMap[uidVector];
		}
	}

	assert(!"Memory place cannot be created for the provided cl_operand.");
	return new MemoryPlace("", true);

}

/**
* @brief Converts @c cl_operand to the instance of the @c MemoryPlace class.
*
* @param[in] operand It will be converted to the @c MemoryPlace object.
* @param[in] indexes It specifies which items of the structure should be converted.
*
* @return @c MemoryPlace instance that was created from @a operand.
*
* Preconditions:
* - @code operand->code == CL_OPERAND_VAR @endcode
*/
MemoryPlace* OperandToMemoryPlace::convert(const cl_operand *operand,
										   deque<int> indexes)
{
	if (indexes.empty()) {
		// Simple variable.
		return OperandToMemoryPlace::convertSimpleOperand(operand);
	}

	// Checks if the precondition is satisfied.
	assert(operand->code == CL_OPERAND_VAR);

	// Stores the name of the variable.
	string name;
	if (((operand->data).var)->name != NULL) {
		// This is necessary because of artificial variables.
		name = string(((operand->data).var)->name);
	}

	// Stores if the variable was generated by the compiler.
	bool artificial = ((operand->data).var)->artificial;

	// Stores the unique id of the variable.
	Int uid = ((operand->data).var)->uid;

	// Vector represents unique id for the given operand.
	vector<Int> uidVector;
	uidVector.push_back(uid);

	const struct cl_type *currentType;
	if (operand->accessor != NULL) {
		currentType = operand->accessor->type;
	} else {
		currentType = operand->type;
	}

	// If the given cl_operand represents a structure or
	// if the given cl_operand represents an array.
	while ((CL_TYPE_STRUCT == currentType->code) ||
			(CL_TYPE_ARRAY == currentType->code)) {

		int index = indexes.front();
		indexes.pop_front();
		uidVector.push_back(index);

		if (CL_TYPE_STRUCT == currentType->code) {
			// If the given cl_operand represents a structure.
			name += string(".");
			name += string(((currentType->items)[index]).name);
		} else if (CL_TYPE_ARRAY == currentType->code) {
			// If the given cl_operand represents an element of an array.
			name += string("[]");
			assert(index == 0);
		}

		currentType = ((currentType->items)[index]).type;
	}

	if (OperandToMemoryPlace::memoryPlaceMap.count(uidVector) == 0) {
		// This variable is used for the first time.
		MemoryPlace *var = new MemoryPlace(name, artificial);
		OperandToMemoryPlace::memoryPlaceMap[uidVector] = var;
		return var;
	} else {
		// This variable was used before. We return found record.
		return OperandToMemoryPlace::memoryPlaceMap[uidVector];
	}

	assert(!"Memory places does not created for provided cl_operand.");
	return new MemoryPlace("", true);
}

/**
* @brief Initializes the @c memoryPlaceMap. Used only for unit tests.
*/
void OperandToMemoryPlace::init()
{
	OperandToMemoryPlace::memoryPlaceMap.clear();
}