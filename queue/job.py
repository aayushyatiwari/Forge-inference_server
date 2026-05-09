import time 
import logging

logger = logging.getLogger(__name__)

# 2 -> urgent
# 1 -> batch
class Job:
    def __init__(self, ID, query, incoming_time=None, priority = 1):
        self.ID = ID 
        self.query = query 
        self.incomingtime = incoming_time if incoming_time is not None else time.time()
        self.starttime = None
        self.priority = priority
        self.endtime = None # not set yet
        self.currPriority = priority
        logger.debug(f"Created job {self.ID} with priority {self.priority}")

    def __repr__(self):
        return f"Job(ID={self.ID}, priority={self.priority}, currPriority={self.currPriority:.2f})"