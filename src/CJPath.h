/*
	MIT License
	
	Copyright (c) 2022 Evgeny Oskolkov (ea dot oskolkov at yandex.ru)
	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**
	@mainpage
	A library that implements extracting elements from JSON using a JSON Path expression. 
*/

#ifndef _CJPATH_H
#define _CJPATH_H

#include <stdio.h>
#include <stdbool.h>

/**
	@brief Specifies that the function is a CJPath interface.
*/
#define CJPATH_API

/**
	@brief Memory Allocation Function Prototype.
*/
typedef void* (*MemAllocFunc)(size_t);

/**
	@brief Memory release function prototype.
*/
typedef void(*MemFreeFunc)(void*);

/**
	@brief Enum containing JSON path processing statuses.
*/
typedef enum _CJPathStatus
{
	/**
	@brief Extraction completed successfully.
	*/
	SUCCESS,

	/**
	@brief Invalid argument passed to function.
	*/
	INVALID_ARGUMENT,

	/**
	@brief JSON is not valid.
	*/
	INVALID_JSON,

	/**
	@brief JSON path is not valid.
	*/
	INVALID_JSON_PATH,

	/**
	@brief Value not found by JSON path.
	*/
	NOT_FOUND,

	/**
	@brief Memory allocation error.
	*/
	BAD_ALLOC
} CJPathStatus;

/**
	@brief The structure containing the extracted data.
*/
typedef struct
{

	/**
		@brief Pointer to result within input.
	*/
	const char* strPtr;

	/**
		@brief Length.
	*/
	size_t strLen;

} CJPathResult;


/**
	@brief A bidirectional list containing the result of extracting data using JSON path.
*/
struct _CJPathList
{
	struct _CJPathList* next;
	struct _CJPathList* prev;
	CJPathResult result;
};

/**
	@brief Alias for _CJPathList.
*/
typedef struct _CJPathList CJPathList;

/**
	@brief Processes the json patch and returns a list of pointers to the occurrences in the original string.
	@param jsonData the string containing the JSON.
	@param jsonDataLen JSON data length.
	@param jsonPath the string containing the JSON path.
	@param jsonPathLen JSON path length.
	@param resultList list containing extracted data.
	@param memAllocFunc memory allocation function.
	@param memFreeFunc memory release function.
	@return Instance of CJPathStatus.
*/
CJPathStatus CJPATH_API CJPathProcessing(const char* jsonData, size_t jsonDataLen, const char* jsonPath,
	size_t jsonPathLen, CJPathList** resultList, MemAllocFunc memAllocFunc, MemFreeFunc memFreeFunc);

/**
	@brief Frees the memory allocated for the result list, including all elements.
	@param resultList list containing extracted data.
	@param memFreeFunc memory release function.
*/
void CJPATH_API CJPathFreeList(CJPathList** resultList, MemFreeFunc memFreeFunc);

#endif // _CJPATH_H