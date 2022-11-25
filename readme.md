# Overview
A library that implements extracting elements from JSON using a JSON Path expression. No conversion to structures or types is used, a pointer to the string (from the input JCLON ) and size is returned.

Implemented in C 99.

# Supported operators

| Operator                  | Description                                                        |
| :------------------------ | :----------------------------------------------------------------- |
| `$`                       | The root element to query. This starts all path expressions.       |
| `*`                       | Wildcard. Available anywhere a name or numeric are required.       |
| `.<name>`                 | Dot-notated child.                                                 |
| `['<name>' (, '<name>')]` | Bracket-notated child or children.                                 |
| `[<index> (, <index>)]`   | Array index or indexes.                                            |
| `[start:end]`             | Array slice operator.                                              |

# Query examples

 - $.obj.arr[0,1]
 - $.obj.arr[*]
 - $['build']
 - $.build
 - $['storage']['build']['level'][1:2]
 - $['storage']['item1','item2']

# Unit test

The set of unit tests is stored in the file main.c. Test frameworks are not used.

# Usage example

``` C
	CJPathStatus status;

	CJPathList* result;
	CJPathList* item;

	result = NULL;

    static const char* json = "{\"id\":[0,1,2,3]}";
    static const char* jsonPath = "$.id[0,2]";

	status = CJPathProcessing(json, strlen(json), jsonPath, strlen(jsonPath), &result, &malloc, &free);
	if (status == SUCCESS)
	{
        for (item = result; item != NULL; item = item->next)
        {
            //
            // Processing item
            //
        }

        CJPathFreeList(&result, &free);
	}
```

# Doxygen documentation

See folder [DoyGenDoc](DoxyGenDoc/).

 # LICENSE

This software is distributed under [MIT](https://opensource.org/licenses/MIT) license.
