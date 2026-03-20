from fastapi import FastAPI
from pydantic import BaseModel
import statistics
import threading

app = FastAPI()
lock = threading.Lock()
values = []
events_processed = 0

class Event(BaseModel):
    source: str
    metric_name: str
    value: float
    timestamp: int = 0

def detect_anomaly(value, vals):
    if len(vals) < 10:
        return False
    mean = statistics.mean(vals)
    stddev = statistics.stdev(vals) or 1.0
    return abs(value - mean) / stddev > 3.0

@app.post("/ingest", status_code=202)
def ingest(event: Event):
    global events_processed
    with lock:
        values.append(event.value)
        if len(values) > 200:
            values.pop(0)
        anomaly = detect_anomaly(event.value, values)
        events_processed += 1
    return {"status": "accepted", "anomaly": anomaly}

@app.get("/health")
def health():
    return {"status": "ok", "events_processed": events_processed}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8081, log_level="error")
