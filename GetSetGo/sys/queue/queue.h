

#ifndef GSG_QUEUE_H_
#define GSG_QUEUE_H_


#include "sys_core.h"


typedef struct{

	uint8_t		*buffer;

	uint16_t	length;
	uint16_t	count;

	uint16_t		todo;
	uint16_t		done;

	uint8_t		unit_size;
	uint8_t		type;		//0=Queue, 1=Stack

	//Gives bugs without padding
	uint16_t	xtra;
}gsg_queueObject_t;

#define QUE_TYP_FIFO	0
#define QUE_TYP_LIFO	1

/*Queue Module Wrappers*/
#define	QUE_Insert(X,Y)				_QUE_Insert(X,Y)
#define	QUE_Delete(X,Y)				_QUE_Delete(X,Y)
#define QUE_Push(X,Y)				_QUE_Insert(X,Y)
#define QUE_Pop(X,Y)				_QUE_Delete(X,Y)
#define QUE_Peek(X,Y)				_QUE_Peek(X,Y,0,1)		//This is the element to be popped out
#define QUE_Peek_fromFront(X,Y,N)	_QUE_Peek(X,Y,N,1)		//This is the element at front of queue (that will be popped out)
#define QUE_Peek_fromTop(X,Y,N)		_QUE_Peek(X,Y,N,1)		//This is the element at top of stack (that will be popped out)
#define QUE_Peek_fromBack(X,Y,N)	_QUE_Peek(X,Y,N,0)	//This is the opposite end element of queue (latest added element)
#define QUE_Peek_fromBottom(X,Y,N)	_QUE_Peek(X,Y,N,0)	//This is the opposite end element of stack (oldest/firstly added element)


bool QUE_Init(gsg_queueObject_t *qObj,void *buffer,uint16_t	length, uint8_t unit_size, uint8_t type);
bool _QUE_Delete(gsg_queueObject_t *qObj, uint8_t *data);
bool _QUE_Insert(gsg_queueObject_t *qObj, uint8_t *data);

#endif
