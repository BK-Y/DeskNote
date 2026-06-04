/* Test for src/config/config.c + src/utils/ini2arr.c */
/* test - plan 11b (Config_Init/Get/Set/Shutdown full suite) */
#include "src/config/config.h"
#include <stdio.h>
#include <string.h>

static int test_count = 0;
static int pass_count = 0;

#define ASSERT(cond, msg) do { \
    test_count++; \
    if (!(cond)) { printf("FAIL [%d]: %s\n", test_count, msg); return 1; } \
    printf("PASS [%d]: %s\n", test_count, msg); \
    pass_count++; \
} while(0)

int main(void)
{
    const char* testfile = "test_config_tmp.ini";

    /* prepare test ini */
    FILE* f = fopen(testfile, "w");
    fprintf(f, "; comment\n");
    fprintf(f, "mode=1\n");
    fprintf(f, "# another comment\n");
    fprintf(f, "width=240\n\n");
    fclose(f);

    /* 1. Config_Init */
    ASSERT(Config_Init(testfile) == 0, "Init");

    /* 2. Config_Get existing */
    int val = Config_Get("mode", -1);
    ASSERT(val == 1, "Get mode");

    /* 3. Config_Get nonexistent */
    val = Config_Get("nonexist", -1);
    ASSERT(val == -1, "Get nonexist returns default");

    /* 4. Config_Set */
    ASSERT(Config_Set("mode", 2) == 0, "Set mode");

    /* 5. Config_Get after Set */
    val = Config_Get("mode", -1);
    ASSERT(val == 2, "Get mode after Set");

    /* 6. Config_Set new key */
    ASSERT(Config_Set("newname", 999) == 0, "Set new key");

    /* 7. Config_Get new key */
    val = Config_Get("newname", -1);
    ASSERT(val == 999, "Get new key");

    /* 8. Check ini file - comments preserved */
    f = fopen(testfile, "r");
    char line[256];
    int found_comment = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "; comment")) found_comment = 1;
    }
    fclose(f);
    ASSERT(found_comment, "comment preserved");

    /* 9. Config_Shutdown */
    Config_Shutdown();

    /* 10. Config_Set after shutdown should fail */
    ASSERT(Config_Set("mode", 3) != 0, "Set after shutdown fails");

    remove(testfile);
    printf("\n%d/%d passed\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
