#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./src/CJPath.h"

/**
	This file contains unit tests.
*/

#define MAX_EXPECTED_CASE 32

typedef struct
{
	unsigned long long offset;
	unsigned long long len;
} ExpectedCase;

typedef struct
{
	const char* json;
	const char* jsonPath;

	CJPathStatus expectedStatus;

	ExpectedCase expectedCase[MAX_EXPECTED_CASE];
	size_t expectedCaseCount;

} TestCase;

bool testFunc(TestCase* testCase)
{
	CJPathStatus status;

	CJPathList* result;
	CJPathList* item;

	size_t idx;
	size_t offset;
	size_t resultCount;

	bool retStatus;

	if (testCase == NULL)
	{
		printf("Invalid argument (testCase pointer is NULL)");
		return false;
	}

	retStatus = true;
	result = NULL;

	// Parse
	status = CJPathProcessing(testCase->json, strlen(testCase->json), testCase->jsonPath, strlen(testCase->jsonPath), &result, &malloc, &free);
	if (status != testCase->expectedStatus)
	{
		printf("Expected(%d) and actual status(%d) are different\n", testCase->expectedStatus, status);
		retStatus = false;
		goto EXIT;
	}


	// Check count
	for (item = result, resultCount=0; item != NULL; item = item->next, ++resultCount);
	if (resultCount != testCase->expectedCaseCount)
	{
		printf("The number of expected(%llu) and actual(%llu) result is different\n", testCase->expectedCaseCount, resultCount);
		retStatus = false;
		goto EXIT;
	}

	// Check result
	for (item = result, idx = 0; item != NULL && idx < testCase->expectedCaseCount; item = item->next, ++idx)
	{
		offset = item->result.strPtr - testCase->json;

		//printf("{%d,%d},", offset, item->result.strLen);
		if (item->result.strLen != testCase->expectedCase[idx].len
			|| offset != testCase->expectedCase[idx].offset)
		{
			printf("Expected and actual offset/length are different. "
				"Offset {expected:%llu, actual %llu}, length {expected:%llu, actual %llu}\n",
				testCase->expectedCase[idx].offset, offset, testCase->expectedCase[idx].len, item->result.strLen);

			retStatus = false;
			goto EXIT;
		}
	}

EXIT:
	if (status == SUCCESS)
		CJPathFreeList(&result, &free);

	return retStatus;
}

static TestCase testCaseArr[] =
{

	[0] = {
		".................", "....", INVALID_JSON_PATH,
	},
	[1] = {
		".................", "$",    INVALID_JSON_PATH,
	},
	[2] = {
		".................", "$]",   INVALID_JSON_PATH,
	},
	[3] = {
		".................", "$[",   INVALID_JSON_PATH,
	},
	[4] = {
		".................", "$.[",  INVALID_JSON_PATH,
	},
	[5] = {
		".................", "$.",   INVALID_JSON_PATH,
	},
	[6] = {
		".................", "w",    INVALID_JSON_PATH,
	},
	[7] = {
		".................", ".w",   INVALID_JSON_PATH,
	},

	[8] = {
		".................", "$['H']",	NOT_FOUND
	},
	[9] = {
		".................", "$.]",    NOT_FOUND
	},
	[10] = {
		".................", "$.w",	NOT_FOUND
	},


	[11] = {
		"{\"store\":		{ \"qwe\":\r\n\t  {\"z\":\r\n256.78, \r\n \"x\":\t\r\n\"he\\\"l\\\"lo\"} } }",
		"$['store']['qwe']['x']",
		SUCCESS,
		{
			{49,11}
		},
		1
	},
	[12] = {
		"{\"store\":		{ \"qwe\":\r\n\t  {\"z\":\r\n256.78, \r\n \"x\":\t\r\n\"hello\"} } }",
		"$['store']['qwe']['x']",
		SUCCESS,
		{
			{49,7},
		},
		1
	},
	[13] = {
		"{\"store\":true}", 
		"$.store", 
		SUCCESS,
		{
			{9, 4}
		},
		1
	},
	[14] = {
		"{\"store\":false}",
		"$.store",
		SUCCESS,
		{
			{9, 5}
		},
		1
	},
	[15] = {
		"{\"store\":null}",
		"$.store",
		SUCCESS,
		{
			{9, 4}
		},
		1
	},


	[16] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}", 
		"$['storage'][*]", 
		SUCCESS,
		{
			{20,12},{41,13},{63,10},{82,45}
		},
		4
	},
	[17] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}", 
		"$['storage']",
		SUCCESS,
		{
			{11,117}
		},
		1
	},
	[18] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}", 
		"$['storage']['item1','item2']",
		SUCCESS,
		{
			{20,12},{41,13},
		},
		2
	},

	
	[19] = {
		"{\"storage\":{\"arr\":[1,2,3],\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]},\"price1\":986434.562345,\"price2\":343452334.2138,\"price3\":283348.9134}}",
		"$['storage'][*]",
		SUCCESS,
		{
			{18,7},{34,45},{89,13},{112,14},{136,11}
		},
		5
	},
	[20] = {
		"{\"storage\":{\"arr\":[1,2,3],\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]},\"price1\":986434.562345,\"price2\":343452334.2138,\"price3\":283348.9134}}",
		"$['storage'][*][*]",
		SUCCESS,
		{
			{19,1},{21,1},{23,1},{39,7},{55,23}
		},
		5
	},


	[21] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage",
		SUCCESS,
		{
			{11,117}
		},
		1
	},
	[22] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$['storage']",
		SUCCESS,
		{
			{11,117}
		},
		1
	},


	[23] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage.item1",
		SUCCESS,
		{
			{20,12}
		},
		1
	},
	[24] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$['storage']['item1']",
		SUCCESS,
		{
			{20,12}
		},
		1
	},

	
	[25] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage.build",
		SUCCESS,
		{
			{82,45}
		},
		1
	},
	[26] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$['storage']['build']",
		SUCCESS,
		{
			{82,45}
		},
		1
	},
	

	[27] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage.build.level",
		SUCCESS,
		{
			{103,23}
		},
		1
	},
	[28] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$['storage']['build']['level']",
		SUCCESS,
		{
			{103,23}
		},
		1
	},
	
	
	[29] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage.build.level[2,0,1]",
		SUCCESS,
		{
			{104,5},{110,8},{119,6}
		},
		3
	},
	[30] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}", 
		"$['storage']['build']['level'][1]", 
		SUCCESS,
		{
			{110,8}
		},
		1
	},

	
	[31] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage.build.level[0:1]",
		SUCCESS,
		{
			{104,5}
		},
		1
	},
	[32] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$['storage']['build']['level'][1:2]",
		SUCCESS,
		{
			{110,8}
		},
		1
	},
	[33] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$['storage']['build']['level'][0:3]",
		SUCCESS,
		{
			{104,5},{110,8},{119,6}
		},
		3
	},


	[34] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage.build.level[*]",
		SUCCESS,
		{
			{104,5},{110,8},{119,6}
		},
		3
	},
	[35] = {
		"{\"storage\":{\"item1\":6456456.2343,\"item2\":546546235.324,\"item3\":324234.324,\"build\":{\"v\":\"value\",\"level\":[\"low\",\"medium\",\"high\"]}}}",
		"$.storage.build.level[*]",
		SUCCESS,
		{
			{104,5},{110,8},{119,6}
		},
		3
	},


	[36] = {
		"{\"build\":		{ \"ver\": {\"n\":123.23, \"s\":\"alpha\"} } }", 
		"$['build']['ver']['s']", 
		SUCCESS,
		{
			{37,7}
		},
		1
	},


	[37] = {
		"{\"build\":		{ \"ver\": {\"n\":123.23, \"s\":\"alpha\"} } }", 
		"$.build.ver.s",
		SUCCESS,
		{
			{37,7}
		},
		1
	},
	[38] = {
		"{\"build\":		\"AlphaVersion\"}",
		"$['build']",
		SUCCESS,
		{
			{11,14}
		},
		1
	},
	[39] = {
		"{\"build\":		\"AlphaVersion\"}",
		"$.build",
		SUCCESS,
		{
			{11,14}
		},
		1
	},


	[40] = {
		"{\"arr\":		{\"str\":[\"helloWorld1\", \"helloWorld2\", \"helloWorld3\", \"helloWorld4\", \"helloWorld5\"]}}",
		"$.arr.str[*]", 
		SUCCESS,
		{
			{17,13},{32,13},{47,13},{62,13},{77,13}
		},
		5
	},
	[41] = {
		"{\"obj\":{\"arr\":[{\"z\":1,	 \"x\":1, 	\"y\":2},{\"z\":2, \"x\":3,\"y\":4},{\"z\":1,\"x\":5,\"y\":6},{\"z\":3,\"x\":7,\"y\":8}]}}",
		"$.obj.arr.*.y",
		SUCCESS,
		{
			{36,1},{57,1},{77,1},{97,1}
		},
		4
	},
	[42] = {
		"{\"obj\":{\"arr\":[{\"y\":88, \"x\":[{\"z\":1},{\"z\":2},{\"z\":3},{\"z\":4}]},{\"y\":88, \"x\":[{\"z\":11},{\"z\":22},{\"z\":33},{\"z\":44}]},{\"y\":88, \"x\":[{\"z\":111},{\"z\":222},{\"z\":333},{\"z\":444}]},{\"y\":88, \"x\":[{\"z\":1111},{\"z\":2222},{\"z\":3333},{\"z\":4444}]]}}",
		"$.obj.arr[*].x[*].z",
		SUCCESS,
		{
			{34,1},{42,1},{50,1},{58,1},{82,2},{91,2},{100,2},{109,2},{134,3},{144,3},{154,3},{164,3},{190,4},{201,4},{212,4},{223,4}
		},
		16
	},
	[43] = {
		"{\"obj\":{\"arr\":[{\"y\":88, \"x\":[{\"z\":1},{\"z\":2},{\"z\":3},{\"z\":4}]},{\"y\":88, \"x\":[{\"z\":11},{\"z\":22},{\"z\":33},{\"z\":44}]},{\"y\":88, \"x\":[{\"z\":111},{\"z\":222},{\"z\":333},{\"z\":444}]},{\"y\":88, \"x\":[{\"z\":1111},{\"z\":2222},{\"z\":3333},{\"z\":4444}]]}}", 
		"$.obj.arr[*].x[*]",
		SUCCESS,
		{
			{29,7},{37,7},{45,7},{53,7},{77,8},{86,8},{95,8},{104,8},{129,9},{139,9},{149,9},{159,9},{185,10},{196,10},{207,10},{218,10}
		},
		16
	},
	

	[44] = {
		"{\"obj\":		{\"arr\":[\"helloWorld1\", \"helloWorld2\", \"helloWorld3\", \"helloWorld4\", \"helloWorld5\"]}}",
		"$.obj.arr[0,1]",
		SUCCESS,
		{
			 {17,13},{32,13}
		},
		2
	},
	[45] = {
		"{\"obj\":{\"arr\":[{\"z\":1,	 \"x\":1, 	\"y\":2},{\"z\":2, \"x\":3,\"y\":4},{\"z\":1,\"x\":5,\"y\":6},{\"z\":3,\"x\":7,\"y\":8}]}}",
		"$.obj.arr[0,1,2].y",
		SUCCESS,
		{
			{36,1},{57,1},{77,1}
		},
		3
	},
	[46] = {
		"{\"obj\":{\"arr\":[{\"y\":88, \"x\":[{\"z\":1},{\"z\":2},{\"z\":3},{\"z\":4}]},{\"y\":88, \"x\":[{\"z\":11},{\"z\":22},{\"z\":33},{\"z\":44}]},{\"y\":88, \"x\":[{\"z\":111},{\"z\":222},{\"z\":333},{\"z\":444}]},{\"y\":88, \"x\":[{\"z\":1111},{\"z\":2222},{\"z\":3333},{\"z\":4444}]]}}", 
		"$.obj.arr[0,1,2].x[0,1,2].z",
		SUCCESS,
		{
			{34,1},{42,1},{50,1},{82,2},{91,2},{100,2},{134,3},{144,3},{154,3}
		},
		9
	},
	

	[47] = {
		"{\"obj\":		{\"arr\":[\"helloWorld1\", \"helloWorld2\", \"helloWorld3\", \"helloWorld4\", \"helloWorld5\"]}}",
		"$.obj.arr[0:1]",
		SUCCESS,
		{
			 {17,13}
		},
		1
	},
	[48] = {
		"{\"obj\":{\"arr\":[{\"z\":1,	 \"x\":1, 	\"y\":2},{\"z\":2, \"x\":3,\"y\":4},{\"z\":1,\"x\":5,\"y\":6},{\"z\":3,\"x\":7,\"y\":8}]}}",
		"$.obj.arr[1:3].y",
		SUCCESS,
		{
			{57,1},{77,1}
		},
		2
	},
	[49] = {
		"{\"obj\":{\"arr\":[{\"y\":88, \"x\":[{\"z\":1},{\"z\":2},{\"z\":3},{\"z\":4}]},{\"y\":88, \"x\":[{\"z\":11},{\"z\":22},{\"z\":33},{\"z\":44}]},{\"y\":88, \"x\":[{\"z\":111},{\"z\":222},{\"z\":333},{\"z\":444}]},{\"y\":88, \"x\":[{\"z\":1111},{\"z\":2222},{\"z\":3333},{\"z\":4444}]]}}", 
		"$.obj.arr[0:2].x[1:2].z",
		SUCCESS,
		{
			{42,1},{91,2},
		},
		2
	},


	[50] = {
		"{\"value\":	123.123123 }",
		"$.value",
		SUCCESS,
		{
			{10,10}
		},
		1
	},
	[52] = {
		"{\"obj\":{\"arr\":[{\"y\":88, \"x\":[{\"z\":1},{\"z\":2},{\"z\":3},{\"z\":4}]},{\"y\":88, \"x\":[{\"z\":11.11},{\"z\":22.22},{\"z\":33.33},{\"z\":44.55}]},{\"y\":88.88, \"x\":[{\"z\":111.111},{\"z\":222.222},{\"z\":333.333},{\"z\":444.444}]},{\"y\":88.88, \"x\":[{\"z\":1111.1111},{\"z\":2222.2222},{\"z\":3333.3333},{\"z\":4444.4444}]]}}",
		"$.obj.arr[*].x[*].z",
		SUCCESS,
		{
			{34,1},{42,1},{50,1},{58,1},{82,5},{94,5},{106,5},{118,5},{149,7},{163,7},{177,7},{191,7},{224,9},{240,9},{256,9},{272,9}
		},
		16
	},
	
	[51] = { 
		"{\"float\":	333.987654", "$.float", INVALID_JSON
	},
	[52] = {
		"{\"obj\":	\"value", "$.obj", INVALID_JSON, 
	},
	[53] = {
		"{\"obj\":	{\"value\":999", "$.obj", INVALID_JSON
	},

};

int main()
{
	size_t count;
	size_t i;
	CJPathStatus CJPstatus;
	int ret;

	count = sizeof(testCaseArr) / sizeof(testCaseArr[0]);
	printf("Number of test: %llu\r\n\r\n", count);
	
	ret = 0;
	printf("BEGIN\r\n");
	for (i = 0; i < count; ++i)
	{
		printf("test %llu. ", i);

		CJPstatus = testFunc(&testCaseArr[i]);
		if (CJPstatus)
			printf("[SUCCESS]\n");
		else 
		{
			printf("[FAILURE]\n");
			ret = 1;
			break;
		}
	}

	printf("DONE\n");
	return ret;
}