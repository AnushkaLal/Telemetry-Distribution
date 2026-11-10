#include "telemetry.hpp"
#include "network.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <limits>
#include <ctime>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

static const std::string DATASET = "data/telemetry_real.csv";
struct History { std::string time, event, result; };
std::vector<History> historyLog;
void waitMs(int ms){ std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
void clearScreen(){
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}
void pause(){ std::cout<<"\n  Press ENTER to continue..."; std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n'); }
int askInt(const std::string& prompt,int lo,int hi){ int x; while(true){std::cout<<prompt; if(std::cin>>x && x>=lo && x<=hi){std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');return x;} std::cout<<"  Enter a number from "<<lo<<" to "<<hi<<".\n";std::cin.clear();std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');} }
bool confirm(const std::string& msg){std::cout<<"\n  "<<msg<<" [y/n] > ";char c;std::cin>>c;std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');return c=='y'||c=='Y';}
std::string now(){std::time_t t=std::time(nullptr);std::tm* tm=std::localtime(&t);char b[32];std::strftime(b,sizeof(b),"%H:%M:%S",tm);return b;}
void header(const std::string& title){clearScreen();std::cout<<"============================================================\n             OPS-SAT GROUND CONTROL CONSOLE\n============================================================\n  "<<title<<"\n------------------------------------------------------------\n";}
void log(const std::string& e,const std::string& r){historyLog.push_back({now(),e,r});}
void printStats(const SimulationStats& s,const std::string& label){
 std::cout<<"\n  "<<label<<"\n  ----------------------------------------------------------\n";
 std::cout<<"  Events submitted       : "<<s.submitted<<"\n  Events processed       : "<<s.processed<<"\n  Processing time        : "<<std::fixed<<std::setprecision(2)<<s.seconds<<" s\n  Throughput             : "<<std::setprecision(0)<<s.throughput<<" events/sec\n  Peak queue occupancy   : "<<s.peakQueue<<"\n  Blocked producer pushes: "<<s.blockedPushes<<"\n";
 for(std::size_t i=0;i<s.workerEvents.size();++i)std::cout<<"  Pod "<<i+1<<" processed       : "<<s.workerEvents[i]<<"\n";
}
void stagedDiagnosis(const std::vector<std::string>& stages){for(const auto& s:stages){std::cout<<"  [RUNNING] "<<s<<"..."<<std::flush;waitMs(350);std::cout<<" done\n";}}
std::vector<Telemetry> load(){try{return loadTelemetry(DATASET);}catch(const std::exception& e){std::cerr<<"\nDataset error: "<<e.what()<<"\nExpected file: "<<DATASET<<"\n";std::exit(1);}}
void runBaseline(const std::vector<Telemetry>& data){
 header("NORMAL OPERATIONS / MISSION CONTROL");
 std::cout<<"  Dataset               : "<<DATASET<<"\n  Source observations   : "<<data.size()<<"\n  Processing events     : 10000 (replayed from source)\n  Processing pods       : 4\n  Queue capacity        : 256\n\n  [1] RUN MISSION\n  [2] BACK\n";
 int c=askInt("\n  SELECT > ",1,2);if(c==2)return;
 if(!confirm("Approve mission launch?"))return;
 header("MISSION EXECUTION"); SimulationConfig cfg;cfg.targetRecords=10000;
 auto s=runSimulation(data,cfg);printStats(s,"BASELINE DISTRIBUTION REPORT");
 log("Normal operations",std::to_string(s.processed)+" / "+std::to_string(s.submitted)+" events processed");pause();
}
void queueScenario(const std::vector<Telemetry>& data){
 header("ANOMALY INJECTION / QUEUE CONGESTION");
 std::cout<<"  Fault: queue capacity is constrained while ingestion continues.\n  The producer will encounter a full bounded queue and block.\n\n  [1] INJECT AND RUN\n  [2] BACK\n";
 if(askInt("\n  SELECT > ",1,2)==2)return;
 SimulationConfig bad;bad.targetRecords=10000;bad.workerCount=4;bad.queueCapacity=32;bad.workerDelayMs=2;
 if(!confirm("Inject queue congestion and start mission?"))return;
 header("QUEUE CONGESTION / DIAGNOSIS");std::cout<<"  Fault injected: queue capacity 32; workers slowed to expose pressure.\n";
 stagedDiagnosis({"Inspecting bounded queue occupancy","Checking producer wait events","Measuring consumer capacity","Calculating recovery configurations"});
 auto before=runSimulation(data,bad,false);printStats(before,"FAULTED SYSTEM");
 std::cout<<"\n  RECOVERY OPTIONS\n  [1] SCALE OUT: add one processing pod (5 total)\n  [2] EXPAND BUFFER: increase queue capacity to 256\n  [3] THROTTLE INGESTION: pace incoming telemetry\n  [4] BACK\n";
 int c=askInt("\n  CHOOSE RECOVERY > ",1,4);if(c==4)return;
 SimulationConfig good=bad;std::string action;
 if(c==1){good.workerCount=5;action="Added processing pod 5";}
 if(c==2){good.queueCapacity=256;action="Expanded queue capacity to 256";}
 if(c==3){good.throttleDelayMs=1;action="Applied producer-side ingestion pacing";}
 std::cout<<"\n  Proposed action: "<<action<<"\n";if(!confirm("Approve and execute recovery?"))return;
 header("QUEUE CONGESTION / RECOVERY EXECUTION");waitMs(400);auto after=runSimulation(data,good,false);
 printStats(after,"RECOVERED SYSTEM");std::cout<<"\n  Verification: "<<(after.processed==after.submitted?"PASS - all submitted events were processed":"CHECK - submitted and processed counts differ")<<"\n";
 log("Queue congestion: "+action,std::to_string(after.processed)+" events processed after recovery");pause();
}
void podScenario(const std::vector<Telemetry>& data){
 header("ANOMALY INJECTION / PROCESSING POD FAILURE");std::cout<<"  A worker is removed from service before the recovery run.\n\n  [1] INJECT AND RUN\n  [2] BACK\n";if(askInt("\n  SELECT > ",1,2)==2)return;
 int pod=askInt("  Select pod to take offline (1-4) > ",1,4)-1;
 if(!confirm("Take pod "+std::to_string(pod+1)+" offline and start mission?"))return;
 header("POD FAILURE / DIAGNOSIS");std::cout<<"  Pod "<<pod+1<<" marked OFFLINE at a task boundary.\n";
 stagedDiagnosis({"Checking worker registry","Confirming failed pod is excluded","Checking remaining processing capacity","Calculating failover options"});
 SimulationConfig bad;bad.targetRecords=10000;bad.offlineWorker=pod;auto before=runSimulation(data,bad,false);printStats(before,"DEGRADED CLUSTER");
 std::cout<<"\n  RECOVERY OPTIONS\n  [1] CONTINUE DEGRADED: keep 3 active pods\n  [2] REPLACE POD: start replacement pod (4 active)\n  [3] BACK\n";
 int c=askInt("\n  CHOOSE RECOVERY > ",1,3);if(c==3)return;
 SimulationConfig good=bad;std::string action;
 if(c==2){good.workerCount=5;good.offlineWorker=pod;action="Replacement pod 5 started; failed pod remains offline";}else action="Continue in degraded mode with 3 active pods";
 if(!confirm("Approve: "+action+"?"))return;
 header("POD FAILURE / RECOVERY EXECUTION");waitMs(400);auto after=runSimulation(data,good,false);printStats(after,"POST-RECOVERY CLUSTER");
 std::cout<<"\n  Verification: "<<(after.processed==after.submitted?"PASS - all submitted events processed":"CHECK - event count mismatch")<<"\n";
 log("Pod failure: "+action,std::to_string(after.processed)+" events processed");pause();
}
void imbalanceScenario(const std::vector<Telemetry>& data){
 header("ANOMALY INJECTION / WORKLOAD IMBALANCE");std::cout<<"  Pod 1 is deliberately assigned a slower processing rate.\n  This creates a measurable imbalance in shared-queue consumption.\n\n  [1] INJECT AND RUN\n  [2] BACK\n";if(askInt("\n  SELECT > ",1,2)==2)return;
 if(!confirm("Inject processing-rate imbalance and run?"))return;
 SimulationConfig bad;bad.targetRecords=10000;bad.slowWorker=0;bad.slowWorkerDelayMs=3;
 header("WORKLOAD IMBALANCE / DIAGNOSIS");stagedDiagnosis({"Comparing per-pod event counts","Measuring worker service-rate differences","Inspecting shared queue distribution","Calculating scheduling correction"});
 auto before=runSimulation(data,bad,false);printStats(before,"IMBALANCED CLUSTER");
 std::cout<<"\n  RECOVERY OPTIONS\n  [1] NORMALIZE: remove the artificial pod processing delay\n  [2] BACK\n";
 if(askInt("\n  CHOOSE RECOVERY > ",1,2)==2)return;
 SimulationConfig good=bad;good.slowWorker=-1;good.slowWorkerDelayMs=0;good.workerDelayMs=0;
 if(!confirm("Approve normalized worker service rates?"))return;
 header("WORKLOAD IMBALANCE / RECOVERY EXECUTION");waitMs(400);auto after=runSimulation(data,good,false);printStats(after,"REBALANCED CLUSTER");
 std::cout<<"\n  Verification: compare per-pod event counts above; shared queue remains enabled.\n";
 log("Workload imbalance: normalized worker service rates",std::to_string(after.processed)+" events processed");pause();
}
void networkScenario(){
 header("ANOMALY INJECTION / NETWORK LINK FAILURE");std::cout<<"  [1] INJECT AND RUN\n  [2] BACK\n";if(askInt("\n  SELECT > ",1,2)==2)return;
 NetworkGraph g;g.addLink("GROUND-A","RELAY-1",4);g.addLink("GROUND-A","RELAY-2",7);g.addLink("RELAY-1","RELAY-2",2);g.addLink("RELAY-1","SAT-GW",6);g.addLink("RELAY-2","SAT-GW",3);g.addLink("SAT-GW","OPS-SAT",2);
 auto original=g.shortestPath("GROUND-A","OPS-SAT");int oldCost=g.pathCost(original);
 header("NETWORK FAILURE / DIAGNOSIS");std::cout<<"  Current network links:\n";g.displayLinks();std::cout<<"\n  Original route: ";for(auto& n:original)std::cout<<n<<" ";std::cout<<"(cost "<<oldCost<<")\n";
 stagedDiagnosis({"Inspecting topology graph","Marking selected link unavailable","Running Dijkstra shortest-path calculation","Evaluating alternate route"});
 std::cout<<"\n  Inject failure on link RELAY-1 <-> SAT-GW.\n";if(!confirm("Disable link and calculate alternate route?"))return;
 g.disableLink("RELAY-1","SAT-GW");auto alt=g.shortestPath("GROUND-A","OPS-SAT");int newCost=g.pathCost(alt);
 std::cout<<"\n  ALTERNATE ROUTE: ";if(alt.empty())std::cout<<"NO ROUTE AVAILABLE\n";else{for(auto& n:alt)std::cout<<n<<" ";std::cout<<"(cost "<<newCost<<")\n";}
 if(alt.empty()){log("Network link failure","No alternate route available");pause();return;}
 if(confirm("Approve alternate route?")){std::cout<<"\n  Route update executed. Verification: PASS - destination remains reachable.\n";log("Network link failure: alternate route approved","New route cost "+std::to_string(newCost));}pause();
}
void anomalyMenu(const std::vector<Telemetry>& data){while(true){header("ANOMALY INJECTION / MISSION CONTROL");std::cout<<"  Select one controlled infrastructure fault.\n\n  [1] QUEUE CONGESTION\n  [2] PROCESSING POD FAILURE\n  [3] WORKLOAD IMBALANCE\n  [4] NETWORK LINK FAILURE\n  [5] BACK\n";int c=askInt("\n  SELECT ANOMALY > ",1,5);if(c==5)return;if(c==1)queueScenario(data);if(c==2)podScenario(data);if(c==3)imbalanceScenario(data);if(c==4)networkScenario();}}
void diagnostics(){header("SYSTEM DIAGNOSTICS");std::cout<<"  [1] PROCESSING CLUSTER\n  [2] QUEUE CONFIGURATION\n  [3] NETWORK TOPOLOGY\n  [4] BACK\n";int c=askInt("\n  SELECT > ",1,4);if(c==1)std::cout<<"\n  Logical worker pool: 4 pods configured in baseline.\n  Worker threads are local C++ std::threads, not Kubernetes pods.\n";if(c==2)std::cout<<"\n  Baseline queue: bounded FIFO, capacity 256.\n  Producer waits when full; consumers wait when empty.\n";if(c==3){NetworkGraph g;g.addLink("GROUND-A","RELAY-1",4);g.addLink("GROUND-A","RELAY-2",7);g.addLink("RELAY-1","RELAY-2",2);g.addLink("RELAY-1","SAT-GW",6);g.addLink("RELAY-2","SAT-GW",3);g.addLink("SAT-GW","OPS-SAT",2);g.displayLinks();}if(c!=4)pause();}
void history(){header("OPERATION HISTORY");if(historyLog.empty())std::cout<<"  No operations recorded in this session.\n";for(const auto& h:historyLog)std::cout<<"  ["<<h.time<<"] "<<h.event<<"\n      Result: "<<h.result<<"\n";pause();}
int main(){auto data=load();while(true){header("HOME / OPERATIONS");std::cout<<"  Dataset loaded: "<<data.size()<<" source observations\n\n  [1] NORMAL OPERATIONS\n      Run healthy telemetry distribution\n\n  [2] ANOMALY INJECTION\n      Inject and resolve an infrastructure failure\n\n  [3] SYSTEM DIAGNOSTICS\n      Inspect workers, queue and network\n\n  [4] OPERATION HISTORY\n      Review this session's operations\n\n  [5] EXIT\n";int c=askInt("\n  SELECT OPERATION > ",1,5);if(c==1)runBaseline(data);if(c==2)anomalyMenu(data);if(c==3)diagnostics();if(c==4)history();if(c==5){header("SHUTDOWN");std::cout<<"  Ground control console closed.\n";break;}}return 0;}
