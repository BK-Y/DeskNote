// Test for src/utils/ini2arr
// test - plan 11b dep (ini2arr module)
#include "src/utils/ini2arr.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char* testfile = "test_ini2arr_tmp.ini";

    // 1. Prepare test file
    FILE* f = fopen(testfile, "w");
    fprintf(f, "; this is a comment\n");
    fprintf(f, "mode=1\n");
    fprintf(f, "# another comment\n");
    fprintf(f, "width=240\n\n");
    fclose(f);

    // 2. Test reading
    Ini2Arr arr;
    if (ini2arr(testfile, &arr) != 0) {
        printf("FAIL: ini2arr returned error\n");
        remove(testfile);
        return 1;
    }
    if (arr.count != 2) {
        printf("FAIL: expected 2 entries, got %d\n", arr.count);
        remove(testfile);
        return 1;
    }
    if (strcmp(arr.entries[0].key, "mode") != 0 || arr.entries[0].val != 1) {
        printf("FAIL: first entry mismatch\n");
        ini2arr_free(&arr);
        remove(testfile);
        return 1;
    }
    if (strcmp(arr.entries[1].key, "width") != 0 || arr.entries[1].val != 240) {
        printf("FAIL: second entry mismatch\n");
        ini2arr_free(&arr);
        remove(testfile);
        return 1;
    }
    printf("PASS: read 2 entries correctly\n");
    ini2arr_free(&arr);

    // 3. Test writing (modify existing key)
    if (ini2arr_write(testfile, "width", 300) != 0) {
        printf("FAIL: ini2arr_write returned error\n");
        remove(testfile);
        return 1;
    }
    if (ini2arr(testfile, &arr) != 0) {
        printf("FAIL: re-read after write error\n");
        remove(testfile);
        return 1;
    }
    if (arr.count != 2 || arr.entries[1].val != 300) {
        printf("FAIL: width should be 300 after write\n");
        ini2arr_free(&arr);
        remove(testfile);
        return 1;
    }
    printf("PASS: modify existing key (width: 240 -> 300)\n");
    ini2arr_free(&arr);

    // 4. Test writing (append new key)
    if (ini2arr_write(testfile, "height", 500) != 0) {
        printf("FAIL: append write error\n");
        remove(testfile);
        return 1;
    }
    if (ini2arr(testfile, &arr) != 0) {
        printf("FAIL: re-read after append error\n");
        remove(testfile);
        return 1;
    }
    if (arr.count != 3) {
        printf("FAIL: expected 3 entries after append, got %d\n", arr.count);
        ini2arr_free(&arr);
        remove(testfile);
        return 1;
    }
    if (strcmp(arr.entries[2].key, "height") != 0 || arr.entries[2].val != 500) {
        printf("FAIL: new entry mismatch\n");
        ini2arr_free(&arr);
        remove(testfile);
        return 1;
    }
    printf("PASS: append new key (height=500)\n");
    ini2arr_free(&arr);

    // 5. Verify comments preserved
    f = fopen(testfile, "r");
    char line[256];
    int found_comment = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "; this is a comment"))
            found_comment = 1;
    }
    fclose(f);
    if (!found_comment) {
        printf("FAIL: comment was not preserved\n");
        remove(testfile);
        return 1;
    }
    printf("PASS: comment preserved\n");

    remove(testfile);
    printf("\nAll tests passed.\n");
    return 0;
}
