#ifndef UTIL_INI2ARR_H
#define UTIL_INI2ARR_H

/**
 * @defgroup utils Utilities
 * Platform-independent utility modules.
 * @{
 */

/**
 * @defgroup ini2arr INI File Array
 * Read/write INI files into a heap-allocated key-value array.
 *
 * @section ini2arr_read Reading
 *
 * Opens the file, reads line by line via fgets. Only lines matching
 * "key=value" are parsed; comment lines and blank lines are ignored.
 * The entries array starts at capacity 16 and grows by doubling (realloc)
 * as needed. One pass, one fopen, one fclose.
 *
 * After reading, only the entries array is kept in memory (heap).
 * Call ini2arr_free() to release it.
 *
 * @section ini2arr_write Writing (row-level)
 *
 * Opens the original file, copies every line to a .tmp file.
 * When a line matching the target key is found, its value is replaced.
 * If the key is not found, a new "key=value" line is appended.
 * After copy, the original file is replaced atomically (remove + rename).
 * Comments, blank lines, and other entries are preserved unchanged.
 *
 * @section ini2arr_limits Limits
 *
 * | Limit | Value | Notes |
 * |-------|-------|-------|
 * | Key length | INI2ARR_KEY_LEN (63) | Longer keys are truncated |
 * | Line length | 256 chars | Longer lines are truncated by fgets |
 * | Entry count | Unlimited | realloc doubles capacity as needed |
 *
 * @section ini2arr_thread Thread safety
 *
 * Not thread-safe. Caller must synchronize if multiple threads access
 * the same file or the same Ini2Arr struct concurrently.
 *
 * Example:
 * @code
 *   Ini2Arr arr;
 *   if (ini2arr("state.ini", &arr) == 0) {
 *       for (int i = 0; i < arr.count; i++)
 *           if (strcmp(arr.entries[i].key, "shell_resident_mode") == 0)
 *               val = arr.entries[i].val;
 *       ini2arr_free(&arr);
 *   }
 *
 *   ini2arr_write("state.ini", "shell_resident_mode", 2);
 * @endcode
 * @{
 */

/** @brief Max key length (bytes, excluding null terminator). */
#define INI2ARR_KEY_LEN 63

/** @brief A single key-value entry parsed from an INI file. */
typedef struct {
    char key[INI2ARR_KEY_LEN + 1];  /**< Key string, null-terminated. */
    int  val;                        /**< Integer value. */
} Ini2ArrEntry;

/** @brief Array of entries parsed from an INI file.
 *  entries is heap-allocated; call ini2arr_free() to release. */
typedef struct {
    Ini2ArrEntry* entries;  /**< Heap-allocated entry array. */
    int           count;    /**< Number of valid entries. */
} Ini2Arr;

/** @brief Read an INI file into entries.
 *  @param path  File path (UTF-8 or system encoding).
 *  @param out   Populated Ini2Arr (entries heap-allocated).
 *  @return 0 on success, non-zero on error.
 *  @note Call ini2arr_free() when done with the returned data. */
int ini2arr(const char* path, Ini2Arr* out);

/** @brief Write a single key=value pair to an INI file.
 *
 *  Row-level write: reads the original file, replaces the line matching
 *  @p key, or appends a new line if not found. Comments and other lines
 *  are preserved. The replacement is atomic via .tmp + rename.
 *
 *  @param path  File path (UTF-8 or system encoding).
 *  @param key   Key to set (must match an existing key for replacement).
 *  @param val   Integer value to assign.
 *  @return 0 on success, non-zero on error. */
int ini2arr_write(const char* path, const char* key, int val);

/** @brief Free entries allocated by ini2arr().
 *  @param arr  Ini2Arr to free (sets entries to NULL, count to 0). */
void ini2arr_free(Ini2Arr* arr);

/** @} */
/** @} */
#endif /* UTIL_INI2ARR_H */
