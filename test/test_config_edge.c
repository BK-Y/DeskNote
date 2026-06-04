/* Edge cases for Config: B-3, C-1, C-2, E-1, E-3 */
#include "src/config/config.h"
#include <stdio.h>
#include <string.h>

static int tc = 0, pc = 0;
#define T(cond, msg) do { tc++; if(!(cond)){ printf("FAIL [%d]: %s\n",tc,msg); return 1; } printf("PASS [%d]: %s\n",tc,msg); pc++; } while(0)

static int callback_fired = 0;
static void on_change(const char* k, int o, int n) { (void)k;(void)o;(void)n; callback_fired++; }

int main(void)
{
    const char* f = "test_edge.ini";

    /* E-1: missing file → graceful failure */
    remove(f);
    T(Config_Init(f) != 0, "E-1: Config_Init missing file returns error");

    /* create test ini */
    FILE* fp = fopen(f, "w");
    fprintf(fp, "; a comment\n");
    fprintf(fp, "mode=1\n");
    fprintf(fp, "width=240\n\n");
    fclose(fp);

    /* C-1: init and check consistency */
    Config_Init(f);
    int mode = Config_Get("mode", -1);
    T(mode == 1, "C-1: memory consistent with ini (mode=1)");
    int w = Config_Get("width", -1);
    T(w == 240, "C-1: memory consistent with ini (width=240)");
    T(Config_Get("nonexist", 99) == 99, "C-1: nonexistent returns default");

    /* E-3: modify, shutdown, re-init, verify persistence */
    Config_Set("mode", 5);
    Config_Shutdown();
    Config_Init(f);
    T(Config_Get("mode", -1) == 5, "E-3: mode=5 persisted across Shutdown/Init");

    /* B-3: same value does not trigger callback */
    callback_fired = 0;
    Config_OnChange(on_change);
    Config_Set("mode", 5);  /* same value */
    T(callback_fired == 0, "B-3: same value does not fire callback");

    /* B-3: different value fires callback */
    Config_Set("mode", 9);
    T(callback_fired == 1, "B-3: changed value fires callback once");

    /* B-3: second different value increments */
    Config_Set("width", 999);
    T(callback_fired == 2, "B-3: callback count increments for each change");

    Config_Shutdown();
    remove(f);
    printf("\n%d/%d passed\n", pc, tc);
    return pc == tc ? 0 : 1;
}
