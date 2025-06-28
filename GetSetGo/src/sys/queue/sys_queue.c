

#include "sys_queue.h"

/***************************
 * 	Queue[5] (target buffer)
 * 	Q[0] -> "Done"/Front of Queue
 * 	Q[1]
 * 	Q[2]
 * 	Q[3]
 * 	Q[4]
 * 	Q[5]
 * 	Q[6]-> "ToDo"/Back of Queue/Top of stack
 * 	Q[7]
 * 	Q[8]
 * 	Q[9]
 * 	--- Roll back to position 0 in case of Queue
 **************************/

bool QUE_Init(gsg_queueObject_t *qObj,void *buffer,uint16_t	length, uint8_t unit_size, uint8_t type)
{

	qObj->length 	= length;
	qObj->unit_size = unit_size;
	qObj->buffer 	= buffer;
	qObj->done 		= 0;		//Initial value
	qObj->todo 		= 0;		//Initial value
	qObj->count		= 0;

	if(type)	//Stack type
		qObj->type = 1;
	else		//Queue type
		qObj->type = 0;

	//printf("\nQinit=%d,%d,%d\n",qObj->length,qObj->unit_size,qObj->type);

	return PASS;
}

bool _QUE_Insert(gsg_queueObject_t *qObj, uint8_t *data)
{
	if(qObj != NULL)
	{
		uint8_t	i;
		/*Insertion happens in the same way for Stack and Queue*/
		if(qObj->count < qObj->length)
		{
			for(i=0 ; i<(qObj->unit_size) ; i++)
			{
				*(qObj->buffer + (qObj->todo * qObj->unit_size) +i) = *(data+i);
			}

			qObj->count++;

			qObj->todo++;

			if(!qObj->type)
				if(qObj->todo == qObj->length)	//Re-start from 0 is allowed only in Queue type
					qObj->todo = 0;

			//printf("\nQi="); for(uint16_t j=0 ; j<(qObj->unit_size*qObj->length) ; j++){if((j%qObj->unit_size)==0)printf("  ",*(qObj->buffer+j)); printf("%X",*(qObj->buffer+j));}
		}
		else if(qObj->count == qObj->length)
		{
			return FAIL;
		}

	}
	else		//Queue Object is empty
	{
		return FAIL;
	}
	return PASS;
}

bool _QUE_Delete(gsg_queueObject_t *qObj, uint8_t *data)
{
	if(qObj != NULL)
	{
		uint8_t	i;

		if(!qObj->type)	//Queue type operation
		{
			if(qObj->count > 0)	//queue is not empty
			{
				for(i=0 ; i<(qObj->unit_size) ; i++)
				{
					*(data+i) = *(qObj->buffer + (qObj->done * qObj->unit_size) +i);
					*(qObj->buffer + (qObj->done * qObj->unit_size) +i) = 0x00;
				}

				qObj->count--;

				qObj->done++;
				if(qObj->done == qObj->length)
					qObj->done = 0;

				//printf("\nQd="); for(uint16_t j=0 ; j<(qObj->unit_size*qObj->length) ; j++) printf("%d ",*(qObj->buffer+j));
			}
			else if(qObj->count == 0)
			{
				return FAIL;
			}
		}
		else		//Stack type operation
		{
			if(qObj->count > 0)	//queue is not empty
			{
				qObj->count--;

				qObj->todo--;
				if(qObj->todo <= 0)
					qObj->todo = 0;

				for(i=0 ; i<(qObj->unit_size) ; i++)
				{
					*(data+i) = *(qObj->buffer + (qObj->todo * qObj->unit_size) +i);
					*(qObj->buffer + (qObj->todo * qObj->unit_size) +i) = 0;
				}

				//printf("\nQd="); for(uint16_t j=0 ; j<(qObj->unit_size*qObj->length) ; j++) printf("%d ",*(qObj->buffer+j));
			}
			else if(qObj->count == 0)
			{
				return FAIL;
			}
		}

	}
	else		//Queue Object is empty
	{
		return FAIL;
	}
	return PASS;
}

bool _QUE_Peek(gsg_queueObject_t *qObj, uint8_t *data, int16_t nth, int8_t dir)	//dir is 1 for Front/Pop end, 0 for back end
{
	if(qObj != NULL)
	{
		if(qObj->count > 0)	//queue is not empty
		{
			uint16_t offset,i;

			if(nth >= qObj->count)	//If asked element is beyond the length of queue
				return FAIL;

			if(!qObj->type)	//Queue type operation
			{
				uint16_t ptr=0;
				if(dir)
				{
					//printf("\nFw=[%d,%d,%d]%d",qObj->todo , qObj->done , qObj->count , nth);
					ptr = qObj->done;
					while(nth)
					{
						ptr++;
						if(ptr >= qObj->length)	//Wrap around
							ptr = 0;
						nth--;
					}
				}
				else
				{
					//printf("Rv=[%d,%d,%d]%d",qObj->todo , qObj->done , qObj->count , nth);
					if(qObj->todo)
						ptr = qObj->todo;
					else
						ptr = qObj->length;

					while(nth)
					{
						ptr--;
						if(ptr == 0)		//Wrap around
							ptr = qObj->length;
						nth--;
					}
					ptr--;
				}
				offset = ptr;//printf("(%d)",offset);
			}
			else			//Stack type operation
			{
				if(dir)					//Give out the poppable element
					offset = qObj->todo - nth - 1;
				else					//Give out the first/oldest element
					offset = 0 + nth;
			}

			offset = offset * qObj->unit_size;

			for(i=0 ; i<(qObj->unit_size) ; i++)
			{
				*(data+i) = *(qObj->buffer + offset +i);
			}
		}
		else if(qObj->count == 0)	//Queue has no elements
			return FAIL;
	}
	else		//Queue Object is empty
		return FAIL;

	return PASS;
}
