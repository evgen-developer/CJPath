#include "CJPath_utils.h"
#include <string.h>

char* strnstr(const char* searchString, const char* inputString, size_t inputStringLen)
{
	size_t searchStrLen;
	size_t idx;

	searchStrLen = strlen(searchString);

	if (inputStringLen >= searchStrLen)
	{
		for (idx = 0; idx < inputStringLen - searchStrLen; ++idx)
		{
			if (strncmp(searchString, inputString + idx, searchStrLen) == 0)
				return (char*) inputString + idx; // Found -> ret entry
		}
	}

	// Not found
	return NULL;
}