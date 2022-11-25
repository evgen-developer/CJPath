/*
	MIT License

	Copyright (c) 2022 Evgeny Oskolkov (ea dot oskolkov at yandex.ru)
	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "CJPath.h"
#include "CJPath_utils.h"
#include <stdlib.h>
#include <string.h>

#define RETURN_NEXT_PARSE(offset) return parsePath(jsonData, jsonDataLen, path + (offset), pathLen - (offset), resultList, memAllocFunc, memFreeFunc)

#define JSON_VALUE_TRUE      "true"
#define JSON_VALUE_TRUE_LEN  (sizeof(JSON_VALUE_TRUE)-1)

#define JSON_VALUE_FALSE     "false"
#define JSON_VALUE_FALSE_LEN (sizeof(JSON_VALUE_FALSE)-1)

#define JSON_VALUE_NULL      "null"
#define JSON_VALUE_NULL_LEN  (sizeof(JSON_VALUE_NULL)-1)

static CJPathList* addResultToList(CJPathList** listPtr, CJPathResult* new, MemAllocFunc memAllocFunc)
{
	CJPathList* newItem;

	if (*listPtr == NULL)
	{
		*listPtr = memAllocFunc(sizeof(CJPathList));
		if (*listPtr == NULL)
			return NULL;
		memset(*listPtr, 0, sizeof(CJPathList));
	}
	CJPathList* list = *listPtr;

	if (list->next == list->prev) // List is empty
	{
		// Fill data
		memcpy(&(list->result), new, sizeof(list->result));
		list->prev = list; // Set previous item

		return list;
	}
	else // List is not empty
	{
		while (list->next != NULL)
			list = list->next;

		// Allocate new item
		newItem = memAllocFunc(sizeof(CJPathList));
		if (newItem == NULL)
			return NULL;

		// Add new item
		list->next = newItem;

		// Set previous item
		newItem->prev = list;

		// Set nullptr for next
		newItem->next = NULL;

		// Fill data
		memcpy(&(newItem->result), new, sizeof(newItem->result));

		return newItem;
	}
}

static bool charIsIntegerNum(const char value)
{
	return (value >= '0' && value <= '9');
}

static bool getRange(const char* value, size_t maxLen, size_t* from, size_t* to)
{
	*from = (size_t)atoll(value);

	value = strnstr(":", value, maxLen);
	if (value == NULL)
		return false;
	++value; // by pass :

	*to = (size_t)atoll(value);

	return true;
}

static bool isRange(const char* inputStr, size_t maxLen)
{
	bool found;
	size_t i;

	for (i = 0, found = 0; i < maxLen; ++i)
	{
		if (found != true && inputStr[i] == ':')
			found = true;

		if (inputStr[i] == ']')
		{
			if (found == true)
				return true;
			break;
		}

	}

	return false;
}

static size_t getOffsetChildByDot(const char* inputStr, size_t maxLength)
{
	size_t offset;

	for (offset = 0; offset < maxLength; ++offset)
	{
		if (inputStr[offset] == '.'
			|| inputStr[offset] == '['
			|| inputStr[offset] == '\0')
			return offset;
	}

	return 0;
}

static CJPathStatus getNumArrayValues(const char* inputStr, size_t maxLen, size_t** arr, size_t* countElements, MemAllocFunc memAllocFunc)
{
	const char* ptr;
	size_t i;

	// Calc numbers of elements
	for (i = 0, *countElements = 0; ; ++i)
	{
		if (!(i < maxLen))
		{
			if (inputStr[i - 1] == ',') // last symbol is ,
				return INVALID_JSON_PATH;

			break;
		}
		if (inputStr[i] == ',')
			++(*countElements);
	}

	++(*countElements);

	*arr = NULL;
	*arr = (size_t*)memAllocFunc((*countElements) * sizeof(size_t));
	if (*arr == NULL)
		return BAD_ALLOC;

	for (i = 0, ptr = inputStr; i < *countElements; ++i)
	{
		(*arr)[i] = atoll(ptr);
		ptr = strstr(ptr, ",");
		if (ptr == NULL)
			break;
		ptr += 1;
	}

	return SUCCESS;
}

static CJPathStatus getStrArrayValues(const char* inputStr, size_t maxLen, CJPathResult** arr, size_t* countElements, MemAllocFunc memAllocFunc)
{
	size_t i;
	const char* ptr;
	const char* s1;
	const char* s2;

	// Calc numbers of elements
	for (i = 0, *countElements = 0; ; ++i)
	{
		if (!(i < maxLen))
		{
			if (inputStr[i - 1] == ',') // last symbol is ,
				return INVALID_JSON_PATH;

			break;
		}
		if (inputStr[i] == ',')
			++(*countElements);
	}

	++(*countElements);

	*arr = NULL;
	*arr = (CJPathResult*)memAllocFunc((*countElements) * sizeof(CJPathResult));
	if (*arr == NULL)
		return BAD_ALLOC;

	for (i = 0, ptr = inputStr; i < *countElements; ++i)
	{
		s1 = strnstr("\'", ptr, maxLen);
		if (s1 == NULL)
			return SUCCESS;
		++s1;
		s2 = strnstr("\'", s1, maxLen - (s1 - inputStr - 1));
		if (s2 == NULL)
			return INVALID_JSON_PATH;

		(*arr + i)->strPtr = s1;
		(*arr + i)->strLen = s2 - s1;

		if (ptr == NULL)
			break;

		ptr = (s2 + 1);
	}

	return SUCCESS;
}

// Processing path and extract result
static CJPathStatus processingPath(const char* jsonPath, size_t jsonPathLen, const char* jsonData,
	size_t jsonDataLen, CJPathResult* result, bool extractFirst, MemAllocFunc memAllocFunc, MemFreeFunc memFreeFunc)
{
	CJPathStatus status;

	char* ptr;
	char* ptrEnd;
	char* buildName;
	unsigned cnt1, cnt2;
	size_t sz;
	const char* const endOfFile = jsonData + jsonDataLen;

	status = SUCCESS;
	buildName = NULL;

	// Build name with "
	if (!extractFirst)
	{
		sz = 3;

		buildName = (char*)memAllocFunc(jsonPathLen + sz);
		if (buildName == NULL)
			return BAD_ALLOC;

		ptr = strnstr("'", jsonPath, jsonPathLen);
		if (ptr == NULL)
			ptr = strnstr(".", jsonPath, jsonPathLen);

		if (ptr == NULL)
			ptr = strnstr("[", jsonPath, jsonPathLen);

		if (ptr == NULL)
			sz += strlen(jsonPath);
		else
			sz += (ptr - jsonPath);

		if (sz > jsonPathLen + 3)
			sz = jsonPathLen + 3;

		buildName[0] = '"';
		memcpy(buildName + 1, jsonPath, sz - 1);
		buildName[sz - 2] = '"';
		buildName[sz - 1] = '\0';

		// Find object
		ptr = strnstr(buildName, jsonData, jsonDataLen);
		if (ptr == NULL || ptr >= endOfFile)
		{
			status = NOT_FOUND;
			goto EXIT;
		}

		// Find :
		ptr = strstr(ptr, ":");
		if (ptr == NULL || ptr >= endOfFile)
		{
			status = NOT_FOUND;
			goto EXIT;
		}
	}
	else
	{
		ptr = (char*)jsonData;
	}

	for (++ptr; ptr != endOfFile; ++ptr)
	{
		if (ptr[0] == '"' || ptr[0] == '{' || ptr[0] == '[' || charIsIntegerNum(ptr[0])
			|| (strncmp(ptr, JSON_VALUE_TRUE, JSON_VALUE_TRUE_LEN) == 0)
			|| (strncmp(ptr, JSON_VALUE_FALSE, JSON_VALUE_FALSE_LEN) == 0)
			|| (strncmp(ptr, JSON_VALUE_NULL, JSON_VALUE_NULL_LEN) == 0))
			break;
	}

	if (ptr >= endOfFile)
	{
		status = NOT_FOUND;
		goto EXIT;
	}

	result->strPtr = ptr;

	// Find end of string
	if (ptr[0] == '"')
	{
		for (ptrEnd = ptr + 1, cnt1 = 0, cnt2 = 0; ptrEnd != endOfFile; ++ptrEnd)
		{
			if (ptrEnd[0] == '\\')
			{
				for (++ptrEnd; ptrEnd != endOfFile; ++ptrEnd)
				{
					if (ptrEnd >= endOfFile)
					{
						status = INVALID_JSON;
						goto EXIT;
					}

					if (ptrEnd[0] == '\\') {
						ptrEnd += 2;
						break;
					}
				}
			}
			else if (ptrEnd[0] == '\"')
				break;
		}

		if (ptrEnd >= endOfFile)
		{
			status = INVALID_JSON;
			goto EXIT;
		}

		result->strLen = (size_t)(ptrEnd - ptr) + 1;
	}

	// Find end of number
	else if (charIsIntegerNum(ptr[0]))
	{
		for (++ptr, result->strLen = 1;
			(charIsIntegerNum(ptr[0]) || ptr[0] == '.')
			&& (ptr != endOfFile)
			&& (!(ptr[0] == ']' || ptr[0] == '}' || ptr[0] == ' ' || ptr[0] == '\t' || ptr[0] == '\n' || ptr[0] == '\r'));
			++ptr, ++result->strLen);

		if (ptr >= endOfFile || ptr[0] == '.')
		{
			result->strLen = 0;
			status = INVALID_JSON;
			goto EXIT;
		}
	}

	// Find end of object
	else if (ptr[0] == '{')
	{
		for (ptrEnd = ptr, cnt1 = 0, cnt2 = 0; ptrEnd != endOfFile; ++ptrEnd)
		{

			if (ptrEnd[0] == '{')
				++cnt1;

			if (ptrEnd[0] == '}')
				++cnt2;

			if (cnt1 == cnt2)
				break;
		}

		if (ptrEnd >= endOfFile)
		{
			status = INVALID_JSON;
			goto EXIT;
		}

		result->strLen = (size_t)(ptrEnd - ptr) + 1;
	}

	// Find end of array
	else if (ptr[0] == '[')
	{
		for (ptrEnd = ptr, cnt1 = 0, cnt2 = 0; ptrEnd != endOfFile; ++ptrEnd)
		{

			if (ptrEnd[0] == '[')
				++cnt1;

			if (ptrEnd[0] == ']')
				++cnt2;

			if (cnt1 == cnt2)
				break;
		}

		if (ptrEnd >= endOfFile)
		{
			status = INVALID_JSON;
			goto EXIT;
		}

		result->strLen = (size_t)(ptrEnd - ptr) + 1;
	}

	else if (strncmp(ptr, JSON_VALUE_TRUE, JSON_VALUE_TRUE_LEN) == 0)
		result->strLen = JSON_VALUE_TRUE_LEN;

	else if (strncmp(ptr, JSON_VALUE_NULL, JSON_VALUE_NULL_LEN) == 0)
		result->strLen = JSON_VALUE_NULL_LEN;

	else if (strncmp(ptr, JSON_VALUE_FALSE, JSON_VALUE_FALSE_LEN) == 0)
		result->strLen = JSON_VALUE_FALSE_LEN;

	else
	{
		if (extractFirst)
			status = NOT_FOUND;
		else
			status = INVALID_JSON;
	}

EXIT:
	if (!extractFirst)
		memFreeFunc(buildName);

	return status;
}

static CJPathStatus parsePath(const char* jsonData, size_t jsonDataLen,
	const char* path, size_t pathLen, CJPathList** resultList, MemAllocFunc memAllocFunc, MemFreeFunc memFreeFunc)
{
	CJPathStatus status;

	size_t pathOffset;
	size_t i, j, count, maxCount;
	size_t* array;
	size_t fromValue, toValue;

	CJPathResult res;
	CJPathResult* arrNames;

	const char* ptrTmp;
	const char* ptrTmp2;

	bool present;

	status = SUCCESS;

	// Child element by . (example: $.name)
	if (pathLen >= 2 && path[0] == '.' && path[1] != '.' && path[1] != '*')
	{
		pathOffset = getOffsetChildByDot(++path, --pathLen);
		if (!pathOffset)
			return INVALID_JSON_PATH;

		status = processingPath(path, pathOffset, jsonData, jsonDataLen, &res, false, memAllocFunc, memFreeFunc);
		if (status != SUCCESS)
			return status;

		if ((pathLen - pathOffset) == 1)  // Only null-terminator left
		{
			if ((*resultList = addResultToList(resultList, &res, memAllocFunc)) == NULL)
				return BAD_ALLOC;
		}

		jsonData = res.strPtr;
		jsonDataLen = res.strLen;

		RETURN_NEXT_PARSE(pathOffset);
	}

	// Child element by [] (example: $['name'])
	else if (pathLen >= 5 && path[0] == '[' && path[1] == '\'')
	{
		ptrTmp = strnstr("[", path, pathLen);
		ptrTmp2 = strnstr("]", path, pathLen);
		pathOffset = ptrTmp2 - path + 1;

		if (ptrTmp == NULL || ptrTmp2 == NULL)
			return INVALID_JSON_PATH;

		status = getStrArrayValues(path, (ptrTmp2 - ptrTmp), &arrNames, &count, memAllocFunc);
		if (status != SUCCESS)
			goto ERR_EXIT;
		if (!count)
		{
			status = INVALID_JSON_PATH;
			goto ERR_EXIT;
		}

		for (i = 0; i < count; ++i)
		{
			status = processingPath(arrNames[i].strPtr, arrNames[i].strLen,
				jsonData, jsonDataLen, &res, false, memAllocFunc, memFreeFunc);
			if (status != SUCCESS)
			{
				goto ERR_EXIT;
				return status;
			}

			if ((pathLen - pathOffset) == 1) // Only '] & null-terminator left
			{
				if ((*resultList = addResultToList(resultList, &res, memAllocFunc)) == NULL)
				{
					status = BAD_ALLOC;
					goto ERR_EXIT;
				}
			}
		}

		if (count == 1)
		{
			jsonData = res.strPtr;
			jsonDataLen = res.strLen;
		}

		memFreeFunc(arrNames);

		RETURN_NEXT_PARSE(pathOffset);

	ERR_EXIT:
		memFreeFunc(arrNames);
		return status;
	}

	// Array range (example: $[0:2])
	else if (pathLen >= 5 && path[0] == '[' && isRange(path + 1, pathLen - 1))
	{
		if (!getRange(++path, --pathLen, &fromValue, &toValue))
			return INVALID_JSON_PATH;

		if (toValue <= fromValue)
			return INVALID_JSON_PATH;

		// Find end
		status = INVALID_JSON_PATH;

		for (pathOffset = 3; pathOffset < pathLen; ++pathOffset)
		{
			if (path[pathOffset] == ']')
			{
				status = SUCCESS;
				break;
			}
		}

		// Check valid path
		if (status != SUCCESS)
			return status;

		// Next path
		path += (pathOffset + 1);
		pathLen -= (pathOffset + 1);

		// Extract items
		for (i = 0; i < toValue; ++i)
		{
			// Find correct value
			for (; jsonDataLen != 0;)
			{
				status = processingPath(NULL, 0, jsonData, jsonDataLen, &res, true, memAllocFunc, memFreeFunc);
				if (status == SUCCESS)
					break;
				else
				{
					// Find
					++jsonData;
					--jsonDataLen;
					continue;
				}
			}

			if (!jsonDataLen)
				break;

			jsonDataLen -= res.strLen;
			jsonData += (res.strPtr - jsonData) + res.strLen;

			if (i >= fromValue)
			{
				// Extract objects from item
				if (pathLen == 1)
				{
					if ((*resultList = addResultToList(resultList, &res, memAllocFunc)) == NULL)
						return BAD_ALLOC;
				}
				else
				{
					status = parsePath(res.strPtr, res.strLen, path, pathLen, resultList, memAllocFunc, memFreeFunc);
					if (status == NOT_FOUND)
					{
						++jsonData;
						--jsonDataLen;
						continue;
					}
					else if (status != SUCCESS)
						return status;
				}
			}
		}

		return SUCCESS;
	}

	// Array (example: $[0,1,2]  $[1])
	else if (pathLen >= 2 && path[0] == '[' && charIsIntegerNum(path[1]))
	{
		status = getNumArrayValues(++path, --pathLen, &array, &maxCount, memAllocFunc);
		if (status != SUCCESS)
			return status;

		for (pathOffset = 1; pathOffset < pathLen; ++pathOffset)
		{
			if (path[pathOffset] == ']')
			{
				break;
			}
		}

		if ((pathOffset + 1) > pathLen)
			pathLen = 1;
		else
		{
			path += pathOffset + 1;
			pathLen -= (pathOffset + 1);
		}

		// Extract
		for (i = 0, count = 0; ; ++i)
		{
			// Check
			for (j = 0, present = 0; j < maxCount; ++j)
			{
				if (array[j] == i)
				{
					present = true;
					break;
				}
			}

			// Find correct value
			for (; jsonDataLen != 0;)
			{
				status = processingPath(NULL, 0, jsonData, jsonDataLen, &res, true, memAllocFunc, memFreeFunc);

				if (status == SUCCESS)
					break;
				else
				{
					// Find
					++jsonData;
					--jsonDataLen;
					continue;
				}
			}

			if (!jsonDataLen)
				break;

			jsonDataLen -= res.strLen;
			jsonData += (res.strPtr - jsonData) + res.strLen;

			// Extract objects from item
			if (present)
			{
				if (pathLen == 1)
				{
					if ((*resultList = addResultToList(resultList, &res, memAllocFunc)) == NULL)
						return BAD_ALLOC;
				}
				else
				{
					status = parsePath(res.strPtr, res.strLen, path, pathLen, resultList, memAllocFunc, memFreeFunc);
					if (status == NOT_FOUND)
					{
						++jsonData;
						--jsonDataLen;
						continue;
					}
					else if (status != SUCCESS)
						return status;
				}
				++count;
			}

		}

		memFreeFunc(array);
	}

	// Child element by .* (example: $.name.*)
	else if ((pathLen >= 2 && path[0] == '.' && path[1] == '*')
		|| (pathLen >= 3 && path[0] == '[' && path[1] == '*' && path[2] == ']'))
	{
		pathOffset = (path[0] == '.') ? 2 : 3;

		path += (pathOffset);
		pathLen -= (pathOffset);

		if (jsonData[0] != '[')
		{
			while (1)
			{
				ptrTmp = jsonData;
				jsonData = strnstr(":", jsonData, jsonDataLen);
				if (jsonData == NULL)
					break;
				jsonDataLen -= (jsonData - ptrTmp);

				status = processingPath(NULL, 0, jsonData, jsonDataLen, &res, true, memAllocFunc, memFreeFunc);
				if (status != SUCCESS)
					return status;

				if (pathLen == 1)
				{
					if ((*resultList = addResultToList(resultList, &res, memAllocFunc)) == NULL)
						return BAD_ALLOC;
					jsonData = res.strPtr + res.strLen;
					jsonDataLen -= res.strLen;
				}
				else
				{
					status = parsePath(res.strPtr, res.strLen, path, pathLen, resultList, memAllocFunc, memFreeFunc);
					if (status == NOT_FOUND) // End
						break;
					jsonData = res.strPtr + res.strLen;
					jsonDataLen -= res.strLen;
				}

			}
		}
		else
		{
			status = processingPath(NULL, 0, jsonData, jsonDataLen, &res, true, memAllocFunc, memFreeFunc);
			if (status != SUCCESS)
				return status;

			// Extract
			while (1)
			{

				// Find correct value
				for (; jsonDataLen != 0;)
				{
					status = processingPath(NULL, 0, jsonData, jsonDataLen, &res, true, memAllocFunc, memFreeFunc);

					if (status == SUCCESS)
						break;
					else
					{
						// Find
						++jsonData;
						--jsonDataLen;
						continue;
					}
				}

				if (!jsonDataLen)
					break;

				jsonDataLen -= res.strLen;
				jsonData += (res.strPtr - jsonData) + res.strLen;

				// Extract objects from item
				if (pathLen == 1)
				{
					if ((*resultList = addResultToList(resultList, &res, memAllocFunc)) == NULL)
						return BAD_ALLOC;
				}
				else
				{
					status = parsePath(res.strPtr, res.strLen, path, pathLen, resultList, memAllocFunc, memFreeFunc);
					if (status == NOT_FOUND)
					{
						++jsonData;
						--jsonDataLen;
						continue;
					}
					else if (status != SUCCESS)
						return status;
				}
			}
		}

		return SUCCESS;
	}

	else if (pathLen != 1)
		return INVALID_JSON_PATH;

	return SUCCESS;
}

CJPathStatus CJPathProcessing(const char* jsonData, size_t jsonDataLen,
	const char* jsonPath, size_t jsonPathLen, CJPathList** resultList, MemAllocFunc memAllocFunc, MemFreeFunc memFreeFunc)
{
	CJPathStatus status;
	if (jsonData == NULL || jsonPath == NULL || resultList == NULL)
		return INVALID_ARGUMENT;

	if (jsonDataLen < 5)
		return INVALID_JSON;

	if (!(jsonPath[0] == '$' && (jsonPath[1] == '[' || jsonPath[1] == '.') && jsonPathLen >= 3))
		return INVALID_JSON_PATH;

	status = parsePath(jsonData, jsonDataLen, jsonPath + 1, jsonPathLen, resultList, memAllocFunc, memFreeFunc);

	// Set list to begin
	if (*resultList != NULL)
	{
		CJPathList* prevPtr;
		do
		{
			prevPtr = (*resultList)->prev;
			*resultList = prevPtr->prev;
		} while (prevPtr != *resultList);
	}

	if (status != SUCCESS)
		CJPathFreeList(resultList, memFreeFunc);

	return status;
}

void CJPathFreeList(CJPathList** list, MemFreeFunc memFreeFunc)
{
	while (*list != NULL)
	{
		CJPathList* tmp = *list;
		*list = (*list)->next;
		memFreeFunc(tmp);
	}

	*list = NULL;
}
