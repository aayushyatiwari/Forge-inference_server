from scheduler import Scheduler
from worker import worker
import threading 
from job import Job
import time
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s | %(levelname)-8s | %(name)s | %(threadName)s | %(message)s',
    handlers=[
        logging.FileHandler("forge.log", mode='w'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    logger.info("Starting Forge Inference Server")
    s = Scheduler()
    
    threads = []
    for i in range(1, 4):
        t = threading.Thread(target=worker, args=(i, s), name=f"WorkerThread-{i}")
        t.start()
        threads.append(t)
        logger.info(f"Started worker thread {i}")

    aging_thread = threading.Thread(target=s.aging_thread, name="AgingThread")
    aging_thread.start()
    threads.append(aging_thread)
    logger.info("Started aging thread")

    job_id = 0
    try:
        for i in range(100):
            job = Job(job_id, f"urgent query {job_id}", priority=2)
            s.add_job(job)
            logger.debug(f"Added urgent job {job_id}")
            job_id += 1
            
            if i % 10 == 0:
                batch_job = Job(job_id, f"batch query {job_id}", priority=1)
                s.add_job(batch_job)
                logger.debug(f"Added batch job {job_id}")
                job_id += 1
            
            time.sleep(0.05)
    except KeyboardInterrupt:
        logger.info("Shutdown requested...")
    
    logger.info("Main loop finished adding jobs.")
