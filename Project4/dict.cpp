//This is our a wrappre for the threaded file
class threadPacket{
	public:
		int * doneFlag;
		sem_t * queueLock;
		std::unordered_map<std::string, int,KeyHasher> * results;
		std::queue<std::string> * q;
};

//This is our thread worker function, it takes from a queue and encodes the data into our hash table
#define WORKERBUFFER 1000
std::atomic_int number = 0;
void * encodeEntries(void * arg){
	threadPacket * tP = (threadPacket*) arg;
	std::queue<std::string> * queue = tP->q;
	std::unordered_map<std::string, int,KeyHasher> * results = tP->results;
	std::string * buffer = new std::string[WORKERBUFFER];
	while(*(tP->doneFlag)){
		if(queue->empty()){
		}else{
			sem_wait(tP->queueLock);
			int x = 0;
			for(int i = 0; i < WORKERBUFFER;i++){
				if(queue->empty()){
					break;
				}
				buffer[i] = queue->front();
				queue->pop();
				x++;
			}
			sem_post(tP->queueLock);
			for(int i = 0; i < x;i++){	
				if((results->count(buffer[i]) == 0)){
					//New entry, add it to the dict
					(*results)[buffer[i]] = number;
					number++;
				}
			}
		}
	}
	return 0;
}

int MAXTHREAD = 1;
#define BUFFERSIZE 10000 //the amount of enteries read in from the file and passed to workers in a single batch
std::unordered_map<std::string,int,KeyHasher> * generateDict(std::ifstream& inputStream){
	
	std::queue<std::string> * inputQueue = new std::queue<std::string>();
	std::unordered_map<std::string, int,KeyHasher> * results = new std::unordered_map<std::string, int,KeyHasher>[MAXTHREAD];

	//Store our threads
	pthread_t T[MAXTHREAD];

	//We are going to use a flag to tell all the threads when there are no more enteries to process
	int doneFlag = 1;
	sem_t queueLock;

	sem_init(&queueLock, 0 , 1);

	//Start by spining off the number of threads we are going to use
	for(int i = 0; i < MAXTHREAD;i++){
		threadPacket * tP = new threadPacket;
		tP->doneFlag = &doneFlag;
		tP->results = &results[i];
		tP->q = inputQueue;
		tP->queueLock = &queueLock;
		int resultCode = pthread_create(&T[i], NULL, encodeEntries, tP);
	}

	//Now we will read through our entire file and pass enteries to our workers
	std::string * buffer = new std::string[BUFFERSIZE];	
	while(inputStream){
		std::string line;
		int x = 0;
		//Buffering it allows thread's to process while we are reading, this assumes file IO is slow
		for(int i = 0; i < BUFFERSIZE; i++){
			if(getline(inputStream,line)){
				buffer[i] = line;
				x++;
			}else{
				break;
			}
		}
		sem_wait(&queueLock);	
		for(int i = 0; i < x; i++){
			inputQueue->push(buffer[i]);
		}
		sem_post(&queueLock);
	}

	//Signal that we are done with the file
	//Wait for all our thread's to join back up
	while(!(inputQueue->empty())){
	}
	doneFlag = 0;
	for(int i = 0; i < MAXTHREAD;i++){
		pthread_join(T[i],NULL);
	}

	//Merge all our results
	int size = 0;
	for(int i = 0; i < MAXTHREAD;i++){
		size += results[i].size();
	}
	std::unordered_map<std::string, int,KeyHasher> * finalResult = new std::unordered_map<std::string, int,KeyHasher>();
	finalResult->reserve(size);
	for(int i = 0; i < MAXTHREAD;i++){
		finalResult->merge(results[i]);
	}
	return finalResult;
}
