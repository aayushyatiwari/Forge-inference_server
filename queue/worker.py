import time
import threading 
import logging

logger = logging.getLogger(__name__)

def worker(worker_id, sch):
    '''
    worker_id: threads id
    sch: is the scheduler object
    '''
    logger.info(f"Worker {worker_id} started")
    while True:
        j = sch.get_next_job()
        if j is not None:
            j.starttime = time.time()
            wait_time = j.starttime - j.incomingtime
            logger.info(f"Worker {worker_id} processing job {j.ID} (priority={j.priority}, currPriority={j.currPriority:.2f}, waited={wait_time:.2f}s)")
            
            time.sleep(0.5) # inference imitation
            
            j.endtime = time.time()
            processing_time = j.endtime - j.starttime
            logger.info(f"Worker {worker_id} finished job {j.ID} in {processing_time:.2f}s")
        else:
            time.sleep(0.1)