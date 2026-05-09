import threading 
import heapq
import time 
import logging

logger = logging.getLogger(__name__)

class Scheduler:
    def __init__(self, aging_rate = 0.2):
        self.heap = []
        self.lock = threading.Lock()
        self.agingRate = aging_rate 
        logger.info(f"Initialized Scheduler with aging_rate={aging_rate}")

    def add_job(self, job):
        with self.lock:
            heapq.heappush(self.heap, (-job.currPriority, job.incomingtime, job))
        logger.debug(f"Job {job.ID} added to queue. Current queue size: {len(self.heap)}")

    def get_next_job(self):
        with self.lock:
            if self.heap:
                _,_, j = heapq.heappop(self.heap)
                logger.debug(f"Popped job {j.ID} from queue. Remaining: {len(self.heap)}")
                return j
            else:
                logger.debug("No jobs in the queue")
                return None

    def __len__(self):
        return len(self.heap)
    
    def aging_thread(self):
        logger.info("Aging thread started")
        while True:
            time.sleep(self.agingRate)
            with self.lock:
                if not self.heap:
                    continue
                
                for i, (p,t,j) in enumerate(self.heap):
                    old_p = j.currPriority
                    j.currPriority += self.agingRate
                    self.heap[i] = (-j.currPriority,t, j)
                
                heapq.heapify(self.heap)
                logger.debug(f"Aged {len(self.heap)} jobs")
