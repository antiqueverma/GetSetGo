
#ifndef GSG_GPIO_H_
#define GSG_GPIO_H_

//#include "sys_core.h"

	//Basic stage to write registers
	#define __SET_BIT__(REG,GROUP,PIN)		((REG##GROUP) |= (1<<PIN))
	#define __RESET_BIT__(REG,GROUP,PIN)	((REG##GROUP) &= ~(1<<PIN))
	#define __TOGGLE_BIT__(REG,GROUP,PIN)	((REG##GROUP) ^= (1<<PIN))
	#define __READ_BIT__(REG,GROUP,PIN)		(((REG##GROUP)>>PIN)&0x01)
	#define __WRITE_BYTE__(REG,GROUP,VALUE)	((REG##GROUP) = (VALUE&0xFF))
	#define __READ_BYTE__(REG,GROUP)		(REG##GROUP)

	//Interim stage for preprocessor expansion
	#define _OUTPUT_PIN_(GROUP,PIN)			__SET_BIT__(DDR,GROUP,PIN)
	#define _INPUT_PIN_(GROUP,PIN)			__RESET_BIT__(DDR,GROUP,PIN)
	#define _SET_PIN_(GROUP,PIN)			__SET_BIT__(PORT,GROUP,PIN)
	#define _RESET_PIN_(GROUP,PIN)			__RESET_BIT__(PORT,GROUP,PIN)
	#define _TOGGLE_PIN_(GROUP,PIN)			__TOGGLE_BIT__(PORT,GROUP,PIN)
	#define _WRITE_GROUP_(GROUP,VAL)		__WRITE_BYTE__(PORT,GROUP,VAL)
	#define _READ_PIN_(GROUP,PIN)			__READ_BIT__(PIN,GROUP,PIN)
	#define _READ_GROUP_(GROUP)				__READ_BYTE__(PIN,GROUP)

	/*User Accessible API*/
	#define GPIO_OUTPUT(SIGNAL)				_OUTPUT_PIN_(SIGNAL)
	#define GPIO_INPUT(SIGNAL)				_INPUT_PIN_(SIGNAL)
	#define GPIO_SET(SIGNAL)				_SET_PIN_(SIGNAL)
	#define GPIO_RESET(SIGNAL)				_RESET_PIN_(SIGNAL)
	#define GPIO_TOGGLE(SIGNAL)				_TOGGLE_PIN_(SIGNAL)
	#define GPIO_READ_PIN(SIGNAL)			_READ_PIN_(SIGNAL)
	#define GPIO_WRITE_PORT(SIGNAL,VALUE)	_WRITE_GROUP_(SIGNAL,VALUE)
	#define GPIO_READ_PORT(SIGNAL)			_READ_GROUP_(SIGNAL)

#endif /* GPIO_H_ */
