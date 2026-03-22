#pragma once
#include <crow.h>
#include "../pipeline/StreamProcessor.hpp"
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <deque>
#include <chrono>

struct AnomalyRecord {
    std::string metric;
    std::string source;
    double value;
    double score;
    std::string votes;
    long long timestamp;
};

class DashboardServer {
public:
    DashboardServer(StreamProcessor& processor, int port = 8090)
        : processor_(processor), port_(port) {}

    void add_anomaly(const std::string& metric, const std::string& source,
                     double value, double score, const std::string& votes) {
        std::lock_guard<std::mutex> lock(anomaly_mutex_);
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        anomalies_.push_front({metric, source, value, score, votes, now});
        if (anomalies_.size() > 50) anomalies_.pop_back();
    }

    void start_async() {
        server_thread_ = std::thread([this]() { run(); });
        server_thread_.detach();
    }

private:
    std::string get_html() {
        return R"DASH(<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<title>StreamForge Dashboard</title>
<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
<style>
@import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Share+Tech+Mono&display=swap');
:root{--cyan:#00f5ff;--purple:#bf00ff;--pink:#ff006e;--green:#00ff88;--yellow:#ffbe0b;--bg:#020408;--card:#060e1a;--border:#0a2a4a;}
*{margin:0;padding:0;box-sizing:border-box;}
body{background:var(--bg);color:#cdd9e5;font-family:'Share Tech Mono',monospace;min-height:100vh;}
body::before{content:'';position:fixed;top:0;left:0;right:0;bottom:0;background-image:linear-gradient(rgba(0,245,255,0.03) 1px,transparent 1px),linear-gradient(90deg,rgba(0,245,255,0.03) 1px,transparent 1px);background-size:40px 40px;pointer-events:none;z-index:0;}
body::after{content:'';position:fixed;top:0;left:0;right:0;height:2px;background:linear-gradient(90deg,transparent,var(--cyan),transparent);animation:scanline 4s linear infinite;pointer-events:none;z-index:999;opacity:0.6;}
@keyframes scanline{0%{top:0}100%{top:100vh}}
.header{background:linear-gradient(90deg,#020d1a,#050a12,#020d1a);border-bottom:1px solid var(--cyan);padding:0 32px;height:64px;display:flex;align-items:center;justify-content:space-between;box-shadow:0 0 30px rgba(0,245,255,0.15);position:relative;z-index:10;}
.logo{display:flex;align-items:center;gap:12px;}
.logo-text{font-family:'Orbitron',sans-serif;font-size:1.3rem;font-weight:900;background:linear-gradient(90deg,var(--cyan),var(--purple));-webkit-background-clip:text;-webkit-text-fill-color:transparent;letter-spacing:2px;}
.logo-icon{font-size:1.5rem;animation:pulse-icon 2s ease-in-out infinite;}
@keyframes pulse-icon{0%,100%{filter:drop-shadow(0 0 4px var(--cyan))}50%{filter:drop-shadow(0 0 14px var(--cyan))}}
.header-right{display:flex;align-items:center;gap:20px;}
.live-indicator{display:flex;align-items:center;gap:8px;font-size:0.75rem;color:var(--green);}
.live-dot{width:8px;height:8px;border-radius:50%;background:var(--green);box-shadow:0 0 8px var(--green);animation:blink 1.5s ease-in-out infinite;}
@keyframes blink{0%,100%{opacity:1}50%{opacity:0.2}}
.version-badge{font-family:'Orbitron',sans-serif;font-size:0.7rem;background:linear-gradient(135deg,var(--purple),var(--cyan));padding:4px 12px;border-radius:2px;color:white;letter-spacing:1px;}
.alert-banner{display:none;background:linear-gradient(90deg,#1a0010,#2d0020,#1a0010);border-bottom:1px solid var(--pink);padding:10px 32px;font-size:0.85rem;color:var(--pink);position:relative;z-index:9;}
.alert-banner.active{display:flex;align-items:center;gap:12px;animation:alert-flash 0.8s ease-in-out infinite;}
@keyframes alert-flash{0%,100%{opacity:1}50%{opacity:0.6}}
.stats-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:16px;padding:20px 24px;position:relative;z-index:1;}
.stat-card{background:var(--card);border:1px solid var(--border);border-radius:4px;padding:24px;position:relative;overflow:hidden;}
.stat-card::before{content:'';position:absolute;top:0;left:0;width:3px;height:100%;}
.stat-card.cyan::before{background:var(--cyan);box-shadow:0 0 10px var(--cyan);}
.stat-card.pink::before{background:var(--pink);box-shadow:0 0 10px var(--pink);}
.stat-card.yellow::before{background:var(--yellow);box-shadow:0 0 10px var(--yellow);}
.stat-label{font-size:0.65rem;letter-spacing:2px;text-transform:uppercase;color:#4a6a8a;margin-bottom:12px;}
.stat-value{font-family:'Orbitron',sans-serif;font-size:2.4rem;font-weight:700;line-height:1;}
.stat-card.cyan .stat-value{color:var(--cyan);text-shadow:0 0 20px rgba(0,245,255,0.5);}
.stat-card.pink .stat-value{color:var(--pink);text-shadow:0 0 20px rgba(255,0,110,0.5);}
.stat-card.yellow .stat-value{color:var(--yellow);text-shadow:0 0 20px rgba(255,190,11,0.5);}
.stat-sub{font-size:0.7rem;color:#3a5a7a;margin-top:8px;letter-spacing:1px;}
.bump{animation:value-bump 0.3s ease;}
@keyframes value-bump{0%{transform:scale(1)}50%{transform:scale(1.12)}100%{transform:scale(1)}}
.content{padding:0 24px 24px;display:grid;gap:16px;position:relative;z-index:1;}
.panel{background:var(--card);border:1px solid var(--border);border-radius:4px;overflow:hidden;}
.panel-header{padding:14px 20px;border-bottom:1px solid var(--border);display:flex;align-items:center;justify-content:space-between;background:linear-gradient(90deg,#060e1a,#08121f);}
.panel-title{font-family:'Orbitron',sans-serif;font-size:0.75rem;letter-spacing:2px;color:var(--cyan);text-transform:uppercase;}
.panel-badge{font-size:0.65rem;padding:2px 8px;border-radius:2px;letter-spacing:1px;}
.panel-body{padding:20px;}
.metrics-list{display:flex;flex-wrap:wrap;gap:10px;min-height:40px;align-items:center;}
.metric-tag{background:rgba(0,245,255,0.05);border:1px solid rgba(0,245,255,0.3);color:var(--cyan);padding:6px 14px;border-radius:2px;font-size:0.75rem;letter-spacing:1px;}
.metric-source{color:#4a6a8a;font-size:0.65rem;}
.anomaly-table{width:100%;border-collapse:collapse;font-size:0.78rem;}
.anomaly-table th{padding:10px 16px;text-align:left;font-size:0.65rem;letter-spacing:2px;color:#3a5a7a;border-bottom:1px solid var(--border);text-transform:uppercase;}
.anomaly-table td{padding:12px 16px;border-bottom:1px solid rgba(10,42,74,0.5);}
.anomaly-row{animation:row-appear 0.5s ease;}
@keyframes row-appear{from{opacity:0;transform:translateX(-10px)}to{opacity:1;transform:translateX(0)}}
.votes-badge{font-family:'Orbitron',sans-serif;font-size:0.7rem;color:var(--pink);text-shadow:0 0 8px rgba(255,0,110,0.5);letter-spacing:2px;}
.metric-cell{color:var(--cyan);}.value-cell{color:var(--yellow);}.score-cell{color:var(--purple);}.time-cell{color:#3a5a7a;font-size:0.7rem;}
.empty-state{text-align:center;color:#1a3a5a;padding:30px;font-size:0.8rem;letter-spacing:1px;}
canvas{max-height:220px;}
</style>
</head>
<body>
<div class='header'>
  <div class='logo'>
    <span class='logo-icon'>&#9889;</span>
    <span class='logo-text'>STREAMFORGE</span>
  </div>
  <div class='header-right'>
    <div class='live-indicator'><div class='live-dot'></div>LIVE &middot; AUTO-REFRESH 5s</div>
    <div class='version-badge'>v2.0</div>
  </div>
</div>
<div class='alert-banner' id='alert-banner'>
  <span>&#9888;</span><span id='alert-text'>ANOMALY DETECTED</span>
</div>
<div class='stats-grid'>
  <div class='stat-card cyan'><div class='stat-label'>// Events Processed</div><div class='stat-value' id='ev-processed'>-</div><div class='stat-sub'>TOTAL INGESTED</div></div>
  <div class='stat-card pink'><div class='stat-label'>// Anomalies Detected</div><div class='stat-value' id='ev-anomalies'>-</div><div class='stat-sub'>AI FLAGGED</div></div>
  <div class='stat-card yellow'><div class='stat-label'>// Events Dropped</div><div class='stat-value' id='ev-dropped'>-</div><div class='stat-sub'>BUFFER OVERFLOW</div></div>
</div>
<div class='content'>
  <div class='panel'>
    <div class='panel-header'>
      <span class='panel-title'>&#9658; Throughput Over Time</span>
      <span class='panel-badge' style='color:var(--cyan);border:1px solid rgba(0,245,255,0.3)'>LIVE FEED</span>
    </div>
    <div class='panel-body'><canvas id='throughputChart'></canvas></div>
  </div>
  <div class='panel'>
    <div class='panel-header'>
      <span class='panel-title'>&#9658; Active Metrics</span>
      <span class='panel-badge' id='metrics-count' style='color:var(--purple);border:1px solid rgba(191,0,255,0.3)'>0 STREAMS</span>
    </div>
    <div class='panel-body'><div class='metrics-list' id='metrics-list'>AWAITING DATA STREAMS...</div></div>
  </div>
  <div class='panel'>
    <div class='panel-header'>
      <span class='panel-title'>&#9658; Anomaly Log</span>
      <span class='panel-badge' id='anomaly-count' style='color:var(--pink);border:1px solid rgba(255,0,110,0.3)'>MONITORING</span>
    </div>
    <table class='anomaly-table'>
      <thead><tr><th>METRIC</th><th>SOURCE</th><th>VALUE</th><th>SCORE</th><th>VOTES</th><th>TIMESTAMP</th></tr></thead>
      <tbody id='anomaly-tbody'><tr><td colspan='6' class='empty-state'>[ NO ANOMALIES DETECTED ]</td></tr></tbody>
    </table>
  </div>
</div>
<script>
const ctx = document.getElementById('throughputChart').getContext('2d');
const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {label:'Events Processed',data:[],borderColor:'#00f5ff',backgroundColor:'rgba(0,245,255,0.08)',fill:true,tension:0.4,pointBackgroundColor:'#00f5ff',pointRadius:3,borderWidth:2},
      {label:'Anomalies',data:[],borderColor:'#ff006e',backgroundColor:'rgba(255,0,110,0.08)',fill:true,tension:0.4,pointBackgroundColor:'#ff006e',pointRadius:3,borderWidth:2}
    ]
  },
  options: {
    responsive:true,
    scales:{
      x:{ticks:{color:'#2a4a6a'},grid:{color:'rgba(10,42,74,0.6)'}},
      y:{ticks:{color:'#2a4a6a'},grid:{color:'rgba(10,42,74,0.6)'},beginAtZero:true}
    },
    plugins:{legend:{labels:{color:'#4a6a8a',boxWidth:12}}},
    animation:{duration:400}
  }
});
let prevAnomalies=0,prevProcessed=0;
function bump(id){const el=document.getElementById(id);el.classList.remove('bump');void el.offsetWidth;el.classList.add('bump');}
async function refresh(){
  try{
    const h=await fetch('/api/health').then(r=>r.json());
    if(h.events_processed!==prevProcessed){document.getElementById('ev-processed').textContent=h.events_processed;bump('ev-processed');prevProcessed=h.events_processed;}
    document.getElementById('ev-dropped').textContent=h.events_dropped;
    if(h.anomalies_detected!==prevAnomalies){
      document.getElementById('ev-anomalies').textContent=h.anomalies_detected;bump('ev-anomalies');
      if(h.anomalies_detected>prevAnomalies){
        const b=document.getElementById('alert-banner');
        document.getElementById('alert-text').textContent='ANOMALY DETECTED - '+h.anomalies_detected+' TOTAL FLAGGED BY AI';
        b.classList.add('active');setTimeout(()=>b.classList.remove('active'),4000);
      }
      prevAnomalies=h.anomalies_detected;
    }
    const now=new Date().toLocaleTimeString();
    chart.data.labels.push(now);
    chart.data.datasets[0].data.push(h.events_processed);
    chart.data.datasets[1].data.push(h.anomalies_detected);
    if(chart.data.labels.length>20){chart.data.labels.shift();chart.data.datasets.forEach(d=>d.data.shift());}
    chart.update();
    const ml=await fetch('/api/metrics-list').then(r=>r.json());
    const mDiv=document.getElementById('metrics-list');
    document.getElementById('metrics-count').textContent=(ml.total||0)+' STREAMS';
    if(ml.metrics&&ml.metrics.length>0){
      mDiv.innerHTML=ml.metrics.map(m=>'<span class=\'metric-tag\'>'+m.name+' <span class=\'metric-source\'>['+m.sources.join(',')+']</span></span>').join('');
    }
    const ad=await fetch('/api/anomalies').then(r=>r.json());
    const tbody=document.getElementById('anomaly-tbody');
    document.getElementById('anomaly-count').textContent=ad.anomalies&&ad.anomalies.length>0?ad.anomalies.length+' EVENTS':'MONITORING';
    if(ad.anomalies&&ad.anomalies.length>0){
      tbody.innerHTML=ad.anomalies.map(a=>'<tr class=\'anomaly-row\'><td class=\'metric-cell\'>'+a.metric+'</td><td>'+a.source+'</td><td class=\'value-cell\'>'+parseFloat(a.value).toFixed(2)+'</td><td class=\'score-cell\'>'+parseFloat(a.score).toFixed(3)+'</td><td><span class=\'votes-badge\'>['+a.votes+']</span></td><td class=\'time-cell\'>'+new Date(a.timestamp).toLocaleTimeString()+'</td></tr>').join('');
    }else{
      tbody.innerHTML='<tr><td colspan=\'6\' class=\'empty-state\'>[ NO ANOMALIES DETECTED ]</td></tr>';
    }
  }catch(e){console.log('err',e);}
}
refresh();setInterval(refresh,5000);
</script>
</body>
</html>)DASH";
    }

    void run() {
        crow::SimpleApp app;
        app.loglevel(crow::LogLevel::Warning);

        CROW_ROUTE(app, "/")
        ([this]() {
            auto res = crow::response(200, get_html());
            res.set_header("Content-Type", "text/html");
            return res;
        });

        CROW_ROUTE(app, "/dashboard")
        ([this]() {
            auto res = crow::response(200, get_html());
            res.set_header("Content-Type", "text/html");
            return res;
        });

        CROW_ROUTE(app, "/api/anomalies")
        ([this]() {
            std::lock_guard<std::mutex> lock(anomaly_mutex_);
            std::ostringstream ss;
            ss << "{\"anomalies\":[";
            for (size_t i = 0; i < anomalies_.size(); i++) {
                auto& a = anomalies_[i];
                ss << "{"
                   << "\"metric\":\"" << a.metric << "\","
                   << "\"source\":\"" << a.source << "\","
                   << "\"value\":" << a.value << ","
                   << "\"score\":" << a.score << ","
                   << "\"votes\":\"" << a.votes << "\","
                   << "\"timestamp\":" << a.timestamp
                   << "}";
                if (i + 1 < anomalies_.size()) ss << ",";
            }
            ss << "]}";
            auto res = crow::response(200, ss.str());
            res.set_header("Content-Type", "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        });

        CROW_ROUTE(app, "/api/health")
        ([this]() {
            std::ostringstream ss;
            ss << "{"
               << "\"status\":\"ok\","
               << "\"events_processed\":" << processor_.events_processed() << ","
               << "\"events_dropped\":" << processor_.events_dropped() << ","
               << "\"anomalies_detected\":" << processor_.anomalies_detected()
               << "}";
            auto res = crow::response(200, ss.str());
            res.set_header("Content-Type", "application/json");
            return res;
        });

        CROW_ROUTE(app, "/api/metrics-list")
        ([this]() {
            auto metrics = processor_.get_active_metrics();
            std::ostringstream ss;
            ss << "{\"metrics\":[";
            for (size_t i = 0; i < metrics.size(); i++) {
                auto sources = processor_.get_sources_for_metric(metrics[i]);
                ss << "{\"name\":\"" << metrics[i] << "\",\"sources\":[";
                for (size_t j = 0; j < sources.size(); j++) {
                    ss << "\"" << sources[j] << "\"";
                    if (j + 1 < sources.size()) ss << ",";
                }
                ss << "]}";
                if (i + 1 < metrics.size()) ss << ",";
            }
            ss << "],\"total\":" << metrics.size() << "}";
            auto res = crow::response(200, ss.str());
            res.set_header("Content-Type", "application/json");
            return res;
        });

        std::cout << "StreamForge dashboard starting on port " << port_ << "\n";
        app.port(port_).run();
    }

    StreamProcessor&          processor_;
    int                       port_;
    std::thread               server_thread_;
    mutable std::mutex        anomaly_mutex_;
    std::deque<AnomalyRecord> anomalies_;
};
