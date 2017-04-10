# Producer_Consumer_Problem
This project involves maintaining synchronization among multiple producers and consumers in a producer-consumer scenario. 

## Manager :
It is the main process that creates the producers and consumer processes. It periodically checks whether the system is in deadlock. The manager process creates a file matrix.txt which holds a matrix with 2 rows (number of resources) and 10 columns (ID of producer and consumer processes). Each entry (i, j) of that matrix can have three values :

- 0 => process i has not requested for queue j or released queue j</br>
- 1 => process i requested for queue j</br>
- 2 => process i acquired lock of queue j</br>

The manager initializes all entries to 0. It also creates all the producer/consumer processes. Manager also creates two message
queues (Buffer-size 10) and share them among those processes.

## Producer : 
The producer’s job is to select one of the queues at random and to insert a random number between 1-50, if the queue is not full. It waits if any other process is currently holding the queue.

## Consumer :
The consumer’s job is to remove the element from the queue, if it’s not empty. It can either consume from one queue with p probability or it can consume from both the queues with probability (1-p). If a consumer consumes from both queues, it follows the following algorithm : Request lock on a random queue q -> Consume item from q -> Request lock on the remaining queue q’ -> Consume item from q’ -> Release lock on q -> Release lock on q’.

We implement two variation of the solution. First one, allowing deadlock (case I), second one (case II), implement the following protocol to prevent deadlock. Suppose the queues have ids as Q0 and Q1. If any consumer decides to consume from both the queues, it always requests for Q0 first. In this case deadlock will never happen as the "circular wait" condition never satisfies.
