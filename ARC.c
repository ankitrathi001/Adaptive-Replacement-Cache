#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define PRINTDEBUGS 0
#define HASHSIZE 1000000

#define MAX(a, b) ( (a) > (b) ? (a) : (b) )
#define MIN(a, b) ( (a) < (b) ? (a) : (b) )

/*
*Global Variable Declarations
*/
unsigned int iCacheSize = 0;
char *fileName;
unsigned int iMissCount = 0;
unsigned int iHitCount = 0;
unsigned int hash[HASHSIZE];

/*
* A Queue Node (Implementation of queue using Doubly Linked List)
*/
struct QNode
{
	unsigned int iPageNumber;  /*the page number stored in this QNode*/
	struct QNode *next; /*Pointer to Next Node*/
	struct QNode *prev; /*Pointer to Previous Node*/
};

/*
*A FIFO Queue using QNodes
*/
struct Queue
{
	unsigned int iCount;  /*Number of frames that have been filled.*/
	unsigned int iNumberOfFrames; /*Total number of frames.*/
	struct QNode *front, *rear;	/*Front and Rear Pointers*/
	char *name;	/*Name of the Queue like MRU, MFU, MRUG, MFUG*/
};

/*
*A ARC Cache consisting of 4 Queues
* mrug (B1)- Most Recently Used Ghost
* mru (T1) - Most Recently Used
* mfu (T2) - Most Frequently Used
* mfug (B2) - Most Frequently Used Ghost
*/
struct ARCCache
{
	unsigned int c;
	float p;
	struct Queue mrug, mru, mfu, mfug;
};

/*
*Global Declarations of cache structure
*/
struct ARCCache cache;

/*
*A utility function to create a new Queue Node. The queue Node
* will store the given 'pageNumber'
*/
struct QNode* newQNode(unsigned int pageNumber)
{
	/*Allocate memory and assign 'pageNumber' for the first cache*/
	struct QNode* temp = (struct QNode *)malloc(sizeof(struct QNode));
	temp->iPageNumber = pageNumber;

	/*Initialize prev and next as NULL*/
	temp->prev = NULL;
	temp->next = NULL;

	return temp;
}

/*
* A utility function to create an empty Queue.
* The queue can have at most 'numberOfFrames' nodes
*/
struct Queue* createQueue(unsigned int numberOfFrames)
{
	struct Queue* queue = (struct Queue *)malloc(sizeof(struct Queue));

	/*The queue is empty*/
	queue->iCount = 0;
	queue->front = NULL;
	queue->rear = NULL;

	/*Number of frames that can be stored in memory*/
	queue->iNumberOfFrames = numberOfFrames;

	return queue;
}

/*
*A function to check if there is slot available to bring
* in a new page into memory.
*/
int isQueueFull(struct Queue* queue)
{
	return queue->iCount == queue->iNumberOfFrames;
}

/*
*A function to check if queue is empty
*/
int isQueueEmpty(struct Queue* queue)
{
	return queue->rear == NULL;
}

/*
*A utility function to delete a frame from queue
*/
void dequeueNode(struct Queue* queue, struct QNode* reqPage)
{
	/* base case */
	if (reqPage == NULL)
	{
		printf("Requested Page is Not Available\n");
		return;
	}
	/*If the node to be removed is the only node in the queue*/
	if ((queue->front == reqPage) && (queue->rear == reqPage))
	{
		queue->front = NULL;
		queue->rear = NULL;
	}
	/*If the node to be removed is the node at the front of the queue/list*/
	else if ((queue->front == reqPage) && (queue->rear != reqPage))
	{
		queue->front = reqPage->next;
		queue->front->prev = NULL;
	}
	/*If the node to be removed is the node at the end of the queue/list*/
	else if ((queue->front != reqPage) && (queue->rear == reqPage))
	{
		queue->rear = reqPage->prev;
		queue->rear->next = NULL;
	}
	/*If the node to be removed is the node in the some other location other than front and rear end*/
	else
	{
		reqPage->prev->next = reqPage->next;
		reqPage->next->prev = reqPage->prev;
	}

	/*Free the node after dequeuing it from the queue*/
	free(reqPage);

	// decrement the number of full frames by 1
	queue->iCount--;
}

/*
* This is a debug function. It is used to print the entire contents of the pages
* currently present in the cache. It prints the elements in all the list of the 
* cache.
*/
void printList()
{
	struct Queue* queue = &cache.mru;
	printf("T1 ::: %s(%u/%u):::", queue->name, queue->iCount, cache.p);
	struct QNode* node = queue->front;
	while (node != NULL)
	{
		printf("%d ", node->iPageNumber);
		node = node->next;
	}
	if (queue->front)
		printf("\tFront=%u", queue->front->iPageNumber);
	if (queue->rear)
		printf("\tRear=%u", queue->rear->iPageNumber);

	printf("\n");
	queue = &cache.mrug;
	printf("B1 ::: %s(%u/%u):::", queue->name, queue->iCount, (cache.c - cache.p));
	node = queue->front;
	while (node != NULL)
	{
		printf("%d ", node->iPageNumber);
		node = node->next;
	}
	if (queue->front)
		printf("\tFront=%u", queue->front->iPageNumber);
	if (queue->rear)
		printf("\tRear=%u", queue->rear->iPageNumber);
	printf("\n");
	queue = &cache.mfu;
	printf("T2 ::: %s(%u/%u):::", queue->name, queue->iCount, (cache.c - cache.p));
	node = queue->front;
	while (node != NULL)
	{
		printf("%d ", node->iPageNumber);
		node = node->next;
	}
	if (queue->front)
		printf("\tFront=%u", queue->front->iPageNumber);
	if (queue->rear)
		printf("\tRear=%u", queue->rear->iPageNumber);
	printf("\n");
	queue = &cache.mfug;
	printf("B2 ::: %s(%u/%u):::", queue->name, queue->iCount, (cache.c - cache.p));
	node = queue->front;
	while (node != NULL)
	{
		printf("%d ", node->iPageNumber);
		node = node->next;
	}
	if (queue->front)
		printf("\tFront=%u", queue->front->iPageNumber);
	if (queue->rear)
		printf("\tRear=%u", queue->rear->iPageNumber);
	printf("\n");
}

/*
*A function to add a page with given 'pageNumber' to queue.
*/
void enqueueMRU(struct Queue* queue, unsigned int pageNumber)
{
	/*Create a new node with given page number, And add the new node to the front of queue*/
	struct QNode* temp = newQNode(pageNumber);
	temp->next = queue->front;

	/*If queue is empty, change both front and rear pointers*/
	if (isQueueEmpty(queue))
	{
		queue->front = temp;
		queue->rear = queue->front;
	}
	/*Else change the front*/
	else
	{
		queue->front->prev = temp;
		queue->front = temp;
	}

	/*increment number of full frames*/
	queue->iCount++;
}

/*
* This function is called when a page with given 'pageNumber' is referenced
* from cache (or memory). There are two cases:
* 1. Frame is not there in memory, then it returns NULL
* 2. Frame is there in memory, then return the pointer to the node
*/
struct QNode* ReferencePage(struct Queue* queue, unsigned int iPageNumber)
{
	struct QNode* reqPage = NULL;

	/*Check if the page or the frame being referenced is already present in the queue of nodes.*/
	struct QNode* tempReqPage = NULL;
	for (tempReqPage = queue->front; tempReqPage; tempReqPage = tempReqPage->next)
	{
		if (tempReqPage->iPageNumber == iPageNumber)
		{
			reqPage = tempReqPage;
			return reqPage;
		}
	}
	return reqPage;
}

/*
* This function is called when a page with given 'pageNumber' is to be 
* moved from one queue to another queue. It takes input of the queue from which
* the page needs to be moved and queue into which the page needs to be inserted.
* Third input the function is the page that needs to be moved.
*/
void moveXToT2(struct Queue* fromQueue, struct Queue* toQueue, struct QNode* reqPage)
{
	/*Remove the requested page from fromQueue Check if this the only element in the fromQueue*/
	if ((fromQueue->front == reqPage) && (fromQueue->rear == reqPage))
	{
		fromQueue->front = NULL;
		fromQueue->rear = NULL;
	}
	/*Remove the requested page from fromQueue, Check if this the first element in the fromQueue*/
	else if ((fromQueue->front == reqPage) && (fromQueue->rear != reqPage))
	{
		fromQueue->front = reqPage->next;
		fromQueue->front->prev = NULL;
	}
	/*Remove the requested page from fromQueue, Check if this the last element in the fromQueue*/
	else if ((fromQueue->front != reqPage) && (fromQueue->rear == reqPage))
	{
		fromQueue->rear = reqPage->prev;
		fromQueue->rear->next = NULL;
	}
	/*Remove the requested page from fromQueue, Check if this is not the first/last element in the fromQueue*/
	else
	{
		reqPage->prev->next = reqPage->next;
		reqPage->next->prev = reqPage->prev;
	}
	/*decrement the number of pages in fromQueue by 1*/
	fromQueue->iCount--;


	reqPage->next = toQueue->front;

	/*If queue is empty, change both front and rear pointers*/
	if (isQueueEmpty(toQueue))
	{
		toQueue->front = reqPage;
		toQueue->rear = toQueue->front;
	}
	/*Else change the front*/
	else  
	{
		toQueue->front->prev = reqPage;
		toQueue->front = reqPage;
	}

	/*Increment the number of pages in toQueue by 1.*/
	toQueue->iCount++;
}

/*
* This function is called when a page with given 'pageNumber' is to be moved from
* T2 to B2 or T1 to B1. Basically this function is used to move the elements out from
* one list and add it to the another list beginning.
*/
void replace(const unsigned int iPageNumber, const float p)
{
	if ((cache.mru.iCount >= 1) && ((cache.mru.iCount > p) || ((NULL != ReferencePage(&cache.mfug, iPageNumber)) && (p == cache.mru.iCount))))
	{
		if (cache.mru.rear)
		{
			moveXToT2(&cache.mru, &cache.mrug, cache.mru.rear);
		}
	}
	else
	{
		if (cache.mfu.rear)
		{
			moveXToT2(&cache.mfu, &cache.mfug, cache.mfu.rear);
		}
	}
}

/* 
* This is the function that is used to Lookup an object with the given key.
* It consists of 4 cases, each case represents the list to which the element 
* or the given page may belong.
*/
void arc_lookup(const unsigned int iKeyPageNumber)
{
	struct QNode* reqPage = NULL;
	if (hash[iKeyPageNumber%HASHSIZE] > 0)
	{
	/*Case 1: Part A: Page Found in MRU (T1)*/
	if (NULL != (reqPage = ReferencePage(&cache.mru, iKeyPageNumber)))
	{
#if PRINTDEBUGS == 1
		//printf("HIT :: Case 1: Part A: Page Found in MRU (T1)\n");
#endif
		/*Increment the hit counter*/
		iHitCount++;

		/*Move xt to MRU position in T2.*/
		moveXToT2(&cache.mru, &cache.mfu, reqPage);
	}
	/*Case 1: Part B: Page Found in MFU (T2)*/
	else if (NULL != (reqPage = ReferencePage(&cache.mfu, iKeyPageNumber)))
	{
#if PRINTDEBUGS == 1
		//printf("HIT :: Case 1: Part B: Page Found in MFU (T2)\n");
#endif
		/*Increment the hit counter*/
		iHitCount++;

		/*Move xt to MRU position in T2.*/
		moveXToT2(&cache.mfu, &cache.mfu, reqPage);
	}
	/*Case 2: Page Found in MRUG (B1).*/
	else if (NULL != (reqPage = ReferencePage(&cache.mrug, iKeyPageNumber)))
	{
#if PRINTDEBUGS == 1
		//printf("MISS :: Case 2: Page Found in MRUG (B1).\n");
#endif
		iMissCount++;
		cache.p = (float)MIN((float)cache.c, (cache.p + MAX((cache.mfug.iCount*1.0) / cache.mrug.iCount, 1.0)));
		replace(iKeyPageNumber, cache.p);

		/*Move xt from B1 to the MRU position in T2.*/
		moveXToT2(&cache.mrug, &cache.mfu, reqPage);
	}
	/*Case 3: Page Found in MFUG (B2).*/
	else if (NULL != (reqPage = ReferencePage(&cache.mfug, iKeyPageNumber)))
	{
#if PRINTDEBUGS == 1
		//printf("MISS :: Case 3: Page Found in MFUG (B2).\n");
#endif
		iMissCount++;
		cache.p = (float)MAX(0.0, (float)(cache.p - MAX((cache.mrug.iCount*1.0) / cache.mfug.iCount, 1.0)));
		replace(iKeyPageNumber, cache.p);

		/*Move xt from B2 to the MRU position in T2.*/
		moveXToT2(&cache.mfug, &cache.mfu, reqPage);
	}
	/*Case 4: Page Not Found in MRU, MRUG, MFU, MFUG.*/
	else
	{
#if PRINTDEBUGS == 1
		//printf("Case 4: Page Not Found in MRU, MRUG, MFU, MFUG.\n");
#endif
		iMissCount++;
		/*Case 4: Part A: L1 has c pages*/
		if ((cache.mru.iCount + cache.mrug.iCount) == cache.c)
		{
			if (cache.mru.iCount < cache.c)
			{
				hash[cache.mrug.rear->iPageNumber % HASHSIZE]--;

				/*Delete LRU page in B1.*/
				dequeueNode(&cache.mrug, cache.mrug.rear);
				replace(iKeyPageNumber, cache.p);
			}
			else
			{
				/*Here B1 is empty. Delete LRU page in T1 (also remove it from the cache).*/
				hash[cache.mru.rear->iPageNumber % HASHSIZE]--;

				/*Delete LRU page in T1.*/
				dequeueNode(&cache.mru, cache.mru.rear);
			}
		}
		/*Case 4: Part B: L1 has less than c pages*/
		else if ((cache.mru.iCount + cache.mrug.iCount) < cache.c)
		{
			if ((cache.mru.iCount + cache.mfu.iCount + cache.mrug.iCount + cache.mfug.iCount) >= cache.c)
			{
				if ((cache.mru.iCount + cache.mfu.iCount + cache.mrug.iCount + cache.mfug.iCount) == (2 * cache.c))
				{
					hash[cache.mfug.rear->iPageNumber % HASHSIZE]--;

					/*Delete LRU page in B2.*/
					dequeueNode(&cache.mfug, cache.mfug.rear);					
				}
				replace(iKeyPageNumber, cache.p);
			}
		}
		/*Fetch xt to the cache and move it to MRU position in T1.*/
		enqueueMRU(&cache.mru, iKeyPageNumber);
		hash[iKeyPageNumber % HASHSIZE]++;
	}
	}
	else
	{
#if PRINTDEBUGS == 1
		//printf("Case 4: Page Not Found in MRU, MRUG, MFU, MFUG.\n");
#endif
		iMissCount++;
		/*Case 4: Part A: L1 has c pages*/
		if ((cache.mru.iCount + cache.mrug.iCount) == cache.c)
		{
			if (cache.mru.iCount < cache.c)
			{
				hash[cache.mrug.rear->iPageNumber % HASHSIZE]--;

				/*Delete LRU page in B1.*/
				dequeueNode(&cache.mrug, cache.mrug.rear);
				replace(iKeyPageNumber, cache.p);
			}
			else
			{
				/*Here B1 is empty. Delete LRU page in T1 (also remove it from the cache).*/
				hash[cache.mru.rear->iPageNumber % HASHSIZE]--;

				/*Delete LRU page in T1.*/
				dequeueNode(&cache.mru, cache.mru.rear);
			}
		}
		/*Case 4: Part B: L1 has less than c pages*/
		else if ((cache.mru.iCount + cache.mrug.iCount) < cache.c)
		{
			if ((cache.mru.iCount + cache.mfu.iCount + cache.mrug.iCount + cache.mfug.iCount) >= cache.c)
			{
				if ((cache.mru.iCount + cache.mfu.iCount + cache.mrug.iCount + cache.mfug.iCount) == (2 * cache.c))
				{
					hash[cache.mfug.rear->iPageNumber % HASHSIZE]--;

					/*Delete LRU page in B2.*/
					dequeueNode(&cache.mfug, cache.mfug.rear);
				}
				replace(iKeyPageNumber, cache.p);
			}
		}

		/*Fetch xt to the cache and move it to MRU position in T1.*/
		enqueueMRU(&cache.mru, iKeyPageNumber);
		hash[iKeyPageNumber % HASHSIZE]++;
	}
}

/*
* This function is called to initialize the values of variable of structure of the queue.
* Count is set to 0, front and rear are set to NULL.
*/
void arc_queue_init(struct Queue *queue, unsigned int numberOfFrames, char *name)
{
	queue->name = name;
	// The queue is empty
	queue->iCount = 0;
	queue->front = NULL;
	queue->rear = NULL;

	// Number of frames that can be stored in memory
	queue->iNumberOfFrames = numberOfFrames;
}

/*
* This function is called to initialize the cache structure.
* In the structure, the cache size is set, the adaptation page is set to 0.
* and then a call to function is made to initialize all the queues as part of
* the structure of cache.
*/
void initARCCache(const unsigned int iCacheSize)
{
	cache.c = iCacheSize;
	cache.p = 0;

	char *mrug = "MRUG";
	char *mru = "MRU";
	char *mfu = "MFU";
	char *mfug = "MFUG";

	/*Initialize the queue for each of the list*/
	arc_queue_init(&cache.mrug, iCacheSize, mrug);
	arc_queue_init(&cache.mru, iCacheSize, mru);
	arc_queue_init(&cache.mfu, iCacheSize, mfu);
	arc_queue_init(&cache.mfug, iCacheSize, mfug);
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
		/*argv[1] is the cache size*/
		/*argv[2] is the Trace File Name*/
		iCacheSize = strtol(argv[1], NULL, 10);
		fileName = argv[2];
	}
	else
	{
		printf("Cache Size and Trace File Name not supplied properly.\nProgram Terminating\n");
		getchar();
		exit(-1);
	}

	printf("Cache Size = %d\n", iCacheSize);
	printf("TraceFile Name = %s\n", fileName);

	/*Initialize cache can hold c pages*/
	initARCCache(iCacheSize);

	fp = fopen(fileName, "r");
	if (fp == NULL)
	{
		printf("Error while opening the file: %s\n", fileName);
		getchar();
		exit(EXIT_FAILURE);
	}

	unsigned int iStartingBlock = 0, iNumberOfBlocks = 0, iIgnore = 0, iRequestNumber = 0;
	unsigned int i = 0;
	unsigned int iTotalRequests = 0;
	time(&start);
	
	while (1)
	{
		if (-1 != fscanf(fp, "%u %u %u %u", &iStartingBlock, &iNumberOfBlocks, &iIgnore, &iRequestNumber))
		{
			//printf("%u-->%u\n", iRequestNumber, cache.mru.iCount);
#if PRINTDEBUGS == 1
			//printf("%u\n", iRequestNumber);
#endif
			//printf("%u\t\t%u/%u\t\t%u/%u\t\t%u/%u\t\t%u/%u\n", iRequestNumber, cache->mru.iCount, cache->mru.iNumberOfFrames, cache->mrug.iCount, cache->mrug.iNumberOfFrames, cache->mfu.iCount, cache->mfu.iNumberOfFrames, cache->mfug.iCount, cache->mfug.iNumberOfFrames);
			for (i = iStartingBlock; i < (iStartingBlock + iNumberOfBlocks); i++)
			{
				iTotalRequests++;
#if PRINTDEBUGS == 1
				printf("***************************************************************\nRequested Page = %u\n***************************************************************\n", i);
#endif
				/*Call to check if page is in cache or not*/
				arc_lookup(i);
#if PRINTDEBUGS == 1
				printList();
#endif
			}
		}
		else
		{
			break;
		}
	}
	
	time(&stop);
	/*Print the important statistics about the result for cache hit and miss ratios*/
	printf("Miss Count = %u\n", iMissCount);
	printf("Hit Count = %u\n", iHitCount);
	//printf("iMissCount + iHitCount = %u\n", iMissCount + iHitCount);
	//printf("Total Requests = %u\n", iTotalRequests);
	printf("Hit Ratio = %5.4f %%\n", ((float)(iHitCount * 100) / (iHitCount + iMissCount)));
	printf("Rounded Hit Ratio = %5.2f %%\n", floor(((float)(iHitCount * 100) / (iHitCount + iMissCount)) * 100 + 0.5) / 100);
	printf("Finished in about %.0f seconds. \n", difftime(stop, start));

	getchar();
	return 0;
}