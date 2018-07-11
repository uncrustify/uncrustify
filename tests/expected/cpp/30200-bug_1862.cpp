#if _MSC_VER < 1300
#define __func__    "???"
#else/* comment 1 */
#define __func__    __FUNCTION__
#endif/* comment 2 */

#if _MSC_VER < 1300
#define __func__    "???"
#else// comment 1
#define __func__    __FUNCTION__
#endif// comment 2
