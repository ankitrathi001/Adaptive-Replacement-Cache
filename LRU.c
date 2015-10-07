#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/*
*Global Variable Declarations
*/
unsigned int iCacheSize = 0;
char *fileName;
unsigned int iMissCount = 0;
unsigned int iHitCount = 0;

/*
* A Queue Node (Implementation of queue using Doubly Linked List)
*/
typedef struct QNode
{
	struct QNode *next;
	struct QNode *prev;
	unsigned int iPageNumber;  // the page number stored in this QNode
} QNode;

/*
*A FIFO Queue using QNodes 
*/
typedef struct Queue
{
	unsigned int iCount;  // Number of frames that have been filled.
	unsigned int iNumberOfFrames; // total number of frames
	QNode *front, *rear;
} Queue;

/* 
*A utility function to create a new Queue Node. The queue Node
* will store the given 'pageNumber'
*/
QNode* newQNode(unsigned int pageNumber)
{
	// Allocate memory and assign 'pageNumber' for the first cache
	QNode* temp = (QNode *)malloc(sizeof(QNode));
	temp->iPageNumber = pageNumber;

	// Initialize prev and next as NULL
	temp->prev = NULL;
	temp->next = NULL;

	return temp;
}

/*
* A utility function to create an empty Queue.
* The queue can have at most 'numberOfFrames' nodes
*/
Queue* createQueue(unsigned int numberOfFrames)
{
	Queue* queue = (Queue *)malloc(sizeof(Queue));

	// The queue is empty
	queue->iCount = 0;
	queue->front = NULL;
	queue->rear = NULL;

	// Number of frames that can be stored in memory
	queue->iNumberOfFrames = numberOfFrames;

	return queue;
}

/* 
*A function to check if there is slot available to bring
* in a new page into memory.
*/
int isQueueFull(Queue* queue)
{
	return queue->iCount == queue->iNumberOfFrames;
}

/*
*A function to check if queue is empty
*/
int isQueueEmpty(Queue* queue)
{
	return queue->rear == NULL;
}

/*
*A utility function to delete a frame from queue
*/
void deQueue(Queue* queue)
{
	if (isQueueEmpty(queue))
	{
		return;
	}

	// If this is the only node in list, then change front
	if (queue->front == queue->rear)
	{
		queue->front = NULL;
	}

	// Change rear and remove the previous rear
	QNode* temp = queue->rear;
	queue->rear = queue->rear->prev;

	if (queue->rear)
	{
		queue->rear->next = NULL;
	}

	free(temp);

	// decrement the number of full frames by 1
	queue->iCount--;
}

/*
*A function to add a page with given 'pageNumber' to queue.
*/
void Enqueue(Queue* queue, unsigned int pageNumber)
{
	// If all frames are full, remove the page at the rear
	if (isQueueFull(queue))
	{
		deQueue(queue);
	}

	// Create a new node with given page number,
	// And add the new node to the front of queue
	QNode* temp = newQNode(pageNumber);
	temp->next = queue->front;

	// If queue is empty, change both front and rear pointers
	if (isQueueEmpty(queue))
	{
		queue->front = temp;
		queue->rear = queue->front;
	}
	else  // Else change the front
	{
		queue->front->prev = temp;
		queue->front = temp;
	}

	// increment number of full frames
	queue->iCount++;
}

/*
* This function is called when a page with given 'pageNumber' is referenced
* from cache (or memory). There are two cases:
* 1. Frame is not there in memory, then bring it into memory and add to the front
*    of queue
* 2. Frame is there in memory, then move the frame to front of queue
*/
void ReferencePage(Queue* queue, unsigned int pageNumber)
{
	QNode* reqPage = NULL;

	//Check if the page or the frame being referenced is already present in
	//the queue of nodes.
	QNode* tempReqPage = NULL;
	for (tempReqPage = queue->front; tempReqPage; tempReqPage = tempReqPage->next)
	{
		if (tempReqPage->iPageNumber == pageNumber)
		{
			reqPage = tempReqPage;
			break;
		}
	}

	// the page is not in cache, bring it into the cache.
	if (reqPage == NULL)
	{
		Enqueue(queue, pageNumber);
		iMissCount++;
	}
	else
	{
		// page is there and not at front, change pointer
		if (reqPage != queue->front)
		{
			// Unlink requested page from its current location in queue.
			reqPage->prev->next = reqPage->next;
			if (reqPage->next)
			{
				reqPage->next->prev = reqPage->prev;
			}

			// If the requested page is rear, then change rear
			// as this node will be moved to front
			if (reqPage == queue->rear)
			{
				queue->rear = reqPage->prev;
				queue->rear->next = NULL;
			}

			// Put the requested page before current front
			reqPage->next = queue->front;
			reqPage->prev = NULL;

			// Change prev of current front
			reqPage->next->prev = reqPage;

			// Change front to the requested page
			queue->front = reqPage;
		}
		//Increment the Hit Counter
		iHitCount++;
	}
}

/*
*Driver program to test above functions
*/
int main(int argc, char **argv)
{
	FILE *fp;
	time_t start, stop;

	if (argc == 3)
	{
		//argv[1] is the cache size
		//argv[2] is the Trace File Name
		iCacheSize = strtol(argv[1], NULL, 10);
		fileName = argv[2];
	}
	else
	{
		printf("Cache Size and Trace File Name not supplied properly.\nProgram Terminating\n");
		exit(-1);
	}
	
	printf("Cache Size = %d\n",iCacheSize);
	printf("TraceFile Name = %s\n",fileName);

	//Create cache can hold c pages
	Queue* q = createQueue(iCacheSize-1);

	fp = fopen(fileName, "r");
	if (fp == NULL)
	{
		printf("Error while opening the file: %s\n", fileName);
		exit(EXIT_FAILURE);
	}

	unsigned int iStartingBlock = 0, iNumberOfBlocks = 0, iIgnore = 0, iRequestNumber = 0;
	unsigned int i = 0;
	time(&start);
	
	while (1)
	{
		if (-1 != fscanf(fp, "%u %u %u %u", &iStartingBlock, &iNumberOfBlocks, &iIgnore, &iRequestNumber))
		{
			//printf("%u\n", iRequestNumber);
			for (i = iStartingBlock; i < (iStartingBlock + iNumberOfBlocks); i++)
			{
				ReferencePage(q, i);
			}
		}
		else
		{
			break;
		}
	}
	
	time(&stop);
	printf("Miss Count = %u\n",iMissCount);
	printf("Hit Count = %u\n", iHitCount);
	//printf("iMissCount + iHitCount = %u\n", iMissCount + iHitCount);
	//printf("Total Requests = %u\n", iTotalRequests);
	printf("Hit Ratio = %5.4f %%\n", ((float)(iHitCount * 100) / (iHitCount + iMissCount)));
	printf("Rounded Hit Ratio = %5.2f %%\n", floor(((float)(iHitCount * 100) / (iHitCount + iMissCount)) * 100 + 0.5) / 100);
	printf("Finished in about %.0f seconds. \n", difftime(stop, start));

	//getchar();
	return 0;
}