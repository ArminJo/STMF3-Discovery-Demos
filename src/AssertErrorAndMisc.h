/*
 * AssertErrorAndMisc.h
 *
 * @date 21.01.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#ifndef ASSERTERRORANDMISC_H_
#define ASSERTERRORANDMISC_H_

#include <stdbool.h>
#ifdef STM32F10X
#include <stm32f1xx.h>
#endif
#ifdef STM32F30X
#include <stm32f3xx.h> // for getLR14()
#endif

// Exception Stack Frame of the Cortex-M3 or Cortex-M4 processor.
struct ExceptionStackFrame {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
#if  defined(__ARM_ARCH_7EM__)
    uint32_t s[16];
#endif
};

#define IN_INTERRUPT_SERVICE_ROUTINE ((__get_IPSR() & 0xFF) != 0)

__attribute__( ( always_inline ))       __STATIC_INLINE uint32_t getLR14(void) {
    uint32_t result;
    __ASM volatile ("MOV %0, lr" : "=r" (result) );
    return (result);
}

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT // if enabled uses appr. 3400 bytes

#define assertParamMessage(expr,wrongParam,message) ((expr) ? (void) 0 : assertFailedParamMessage((uint8_t *)__FILE__, __LINE__, getLR14(), (int)wrongParam, message))

#define assertParam(expr,wrongParam) ((expr) ? 0 : assertFailedParamMessage((uint8_t *)__FILE__, __LINE__, getLR14(), (int)wrongParam, StringEmpty))
#else
#define assertParamMessage(expr,wrongParam,aMessage) ((void)0)
#define assertParam(expr,wrongParam) ((void)0)
#endif /* USE_FULL_ASSERT */

#define failParamMessage(wrongParam,message) (assertFailedParamMessage((uint8_t *)__FILE__, __LINE__,getLR14(),(int)wrongParam ,message))
#define Error_Handler() (printError((uint8_t *)__FILE__, __LINE__))

/* Exported functions ------------------------------------------------------- */

void printStackpointerAndStacktrace(int aStackEntriesToSkip, int aStartLine);

#ifdef __cplusplus
extern "C" {
#endif
void FaultHandler(struct ExceptionStackFrame * aFaultArgs);
void assertFailedParamMessage(uint8_t* aFile, uint32_t aLine, uint32_t aLinkRegister, int aWrongParameter, const char * aMessage);
void printError(uint8_t* aFile, uint32_t aLine);
void errorMessage(const char * aMessage);
#ifdef __cplusplus
}
#endif
#define ASSERT_START_Y 200

extern int sLockCount; // counts skipped drawChars because of display locks

extern int DebugValue1;
extern int DebugValue2;
extern int DebugValue3;
extern int DebugValue4;
extern int DebugValue5;

#endif /* ASSERTERRORANDMISC_H_ */
