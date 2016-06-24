#define SOME_MACRO TemplateClass<T>
int i;
#if defined(_MSC_VER)
    #if _MSC_VER < 1300
        #define __func__ "unknown function"
    #else
        #define __func__ __FUNCTION__
    #endif /* _MSC_VER < 1300 */
#endif /* defined(_MSC_VER) */

#define bug_demo        (1 > 2) ? (1 : 2)
