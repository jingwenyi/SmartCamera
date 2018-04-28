/**
 * 项目断言使用到的宏定义。
 */

#include <NkUtils/types.h>
#include <NkUtils/log.h>
#include <string.h>

#ifndef NK_ASSERT_H_
#define NK_ASSERT_H_
NK_CPP_EXTERN_BEGIN


/**
 * 代码运行时检查点。
 */
#define NK_CHECK_POINT() \
	do {\
		NK_PChar file = strrchr(__FILE__, '/');\
		printf("\r\n--  @  %s:%d \r\n\r\n", !file ? __FILE__ : file + 1, __LINE__);\
	} while(0)

/**
 * 代码运行时断言。
 */
#define NK_ASSERT(__exp) \
	do{\
		if(!(__exp)){\
			NK_Log()->alert("\"%s()\" Assertion Condition ( \"%s\" ) @ %d.", __PRETTY_FUNCTION__, #__exp, __LINE__);\
			exit(1);\
		}\
	}while(0)

#define NK_ASSERT_TRUE(__condition) \
	NK_ASSERT(__condition)

#define NK_ASSERT_FALSE(__condition) \
	NK_ASSERT(!(__condition))


#define NK_TEST_(__condition, __verbose) \
	((__condition) ? NK_True : \
			(((__verbose) ? NK_Log()->warn("\"%s() @ %d\" Expect Condition ( \"%s\" ).", __PRETTY_FUNCTION__, __LINE__, #__condition) : 0), NK_False))

#define NK_TEST(__condition) \
	NK_TEST_(__condition, NK_True)

#define NK_EXPECT(__condition) \
	do{\
		if (!NK_TEST_(__condition, NK_False)){\
			return;\
		}\
	}while(0)

#define NK_EXPECT_RETURN(__condition, __ret) \
	do{\
		if (!NK_TEST_(__condition, NK_False)){\
			return(__ret);\
		}\
	}while(0)

#define NK_EXPECT_JUMP(__condition, __location) \
	do{\
		if (!NK_TEST_(__condition, NK_False)){\
			goto __location;\
		}\
	}while(0)


#define NK_EXPECT_VERBOSE(__condition) \
	do{\
		if (!NK_TEST_(__condition, NK_True)){\
			return;\
		}\
	}while(0)

#define NK_EXPECT_VERBOSE_RETURN(__condition, __ret) \
	do{\
		if (!NK_TEST_(__condition, NK_True)){\
			return(__ret);\
		}\
	}while(0)

#define NK_EXPECT_VERBOSE_JUMP(__condition, __location) \
	do{\
		if (!NK_TEST_(__condition, NK_True)){\
			goto __location;\
		}\
	}while(0)


NK_CPP_EXTERN_END
#endif /* NK_ASSERT_H_ */
