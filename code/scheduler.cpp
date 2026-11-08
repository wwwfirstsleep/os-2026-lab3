// 操作系统实验三  处理器调度算法模拟实现与比较
// 王旭升  24281204  保密2401
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

// Single CPU; independent I/O timers; CPU/I/O bursts alternate and end in CPU.
struct Process {
    string id;
    int arrival, priority;
    vector<int> bursts;
};
struct PCB {
    int state = 0; // NEW=0, READY=1, RUNNING=2, BLOCKED=3, DONE=4
    int burst = 0, remaining = 0, wake = -1;
    int first = -1, finish = -1, waiting = 0;
};
struct Result {
    vector<PCB> pcb;
    vector<string> timeline;
    double turnaround = 0, weighted = 0, waiting = 0, response = 0;
    int busy = 0, elapsed = 0, dispatches = 0;
};
const vector<string> algorithms = {"FCFS", "SJF", "PRIORITY", "RR"};

void validate(const vector<Process>& ps) {
    if (ps.empty() || ps.size() > 100) throw runtime_error("Process count must be 1..100");
    long long horizon = 0;
    vector<string> ids;
    for (const auto& p : ps) {
        if (p.id.empty() || find(ids.begin(), ids.end(), p.id) != ids.end())
            throw runtime_error("Process IDs must be unique");
        ids.push_back(p.id);
        if (p.arrival < 0 || p.priority < 0 || p.bursts.empty() || p.bursts.size() % 2 == 0)
            throw runtime_error("Invalid arrival, priority or burst count");
        horizon += p.arrival;
        for (int b : p.bursts) {
            if (b <= 0) throw runtime_error("Burst lengths must be positive");
            horizon += b;
        }
    }
    if (horizon > 1000000) throw runtime_error("Simulation horizon exceeds 1000000");
}
int cpuTotal(const Process& p) {
    int n = 0;
    for (size_t k = 0; k < p.bursts.size(); k += 2) n += p.bursts[k];
    return n;
}

Result simulate(const vector<Process>& ps, const string& alg, int quantum, ostream* log = nullptr) {
    validate(ps);
    if (quantum <= 0) throw runtime_error("Quantum must be positive");
    if (find(algorithms.begin(), algorithms.end(), alg) == algorithms.end())
        throw runtime_error("Unknown algorithm");
    Result r;
    r.pcb.resize(ps.size());
    vector<int> ready;
    int running = -1, pending = -1, slice = 0, done = 0, t = 0;
    auto event = [&](int time, const string& text) {
        if (log) *log << "t=" << setw(3) << time << " " << text << '\n';
    };
    auto enqueue = [&](int i) { r.pcb[i].state = 1; ready.push_back(i); };
    while (done < static_cast<int>(ps.size())) {
        // Arrivals and I/O completions at t precede the expired RR process.
        // Simultaneous external events follow input order.
        for (size_t i = 0; i < ps.size(); ++i) {
            auto& p = r.pcb[i];
            if (p.state == 0 && ps[i].arrival == t) {
                p.remaining = ps[i].bursts[0]; enqueue(static_cast<int>(i));
                event(t, "CREATE " + ps[i].id + " -> READY");
            } else if (p.state == 3 && p.wake == t) {
                enqueue(static_cast<int>(i)); event(t, "IO_DONE " + ps[i].id + " -> READY");
            }
        }
        if (pending >= 0) { enqueue(pending); pending = -1; }
        if (running < 0 && !ready.empty()) {
            size_t pos = 0;
            for (size_t k = 1; k < ready.size(); ++k) {
                int a = ready[k], b = ready[pos];
                if ((alg == "SJF" && r.pcb[a].remaining < r.pcb[b].remaining) ||
                    (alg == "PRIORITY" && ps[a].priority < ps[b].priority)) pos = k;
            }
            running = ready[pos]; ready.erase(ready.begin() + static_cast<long>(pos));
            auto& p = r.pcb[running]; p.state = 2; slice = 0;
            if (p.first < 0) p.first = t;
            ++r.dispatches; event(t, "DISPATCH " + ps[running].id);
        }
        if (log) {
            *log << "t=" << setw(3) << t << " RUN=" << (running < 0 ? "IDLE" : ps[running].id);
            *log << " READY=[";
            for (size_t k = 0; k < ready.size(); ++k) *log << (k ? "," : "") << ps[ready[k]].id;
            *log << "]";
            for (auto pair : vector<pair<int,string>>{{0,"NEW"},{3,"BLOCKED"},{4,"DONE"}}) {
                *log << " " << pair.second << "=[";
                bool first = true;
                for (size_t i = 0; i < ps.size(); ++i) if (r.pcb[i].state == pair.first) {
                    *log << (first ? "" : ",") << ps[i].id; first = false;
                }
                *log << "]";
            }
            *log << '\n';
        }
        for (int i : ready) ++r.pcb[i].waiting;
        r.timeline.push_back(running < 0 ? "IDLE" : ps[running].id);
        if (running >= 0) {
            auto& p = r.pcb[running]; --p.remaining; ++slice; ++r.busy;
            if (p.remaining == 0) {
                if (p.burst + 1 == static_cast<int>(ps[running].bursts.size())) {
                    p.state = 4; p.finish = t + 1; ++done;
                    event(t + 1, "EXIT " + ps[running].id);
                } else {
                    p.wake = t + 1 + ps[running].bursts[p.burst + 1];
                    p.burst += 2; p.remaining = ps[running].bursts[p.burst]; p.state = 3;
                    event(t + 1, "BLOCK " + ps[running].id + " wake=" + to_string(p.wake));
                }
                running = -1;
            } else if (alg == "RR" && slice == quantum) {
                event(t + 1, "QUANTUM " + ps[running].id + " -> READY");
                pending = running; running = -1;
            }
        }
        ++t;
    }
    r.elapsed = t;
    for (size_t i = 0; i < ps.size(); ++i) {
        int turn = r.pcb[i].finish - ps[i].arrival;
        r.turnaround += turn;
        r.weighted += static_cast<double>(turn) / cpuTotal(ps[i]);
        r.waiting += r.pcb[i].waiting;
        r.response += r.pcb[i].first - ps[i].arrival;
        int io = 0;
        for (size_t k = 1; k < ps[i].bursts.size(); k += 2) io += ps[i].bursts[k];
        if (turn != cpuTotal(ps[i]) + io + r.pcb[i].waiting)
            throw runtime_error("Time conservation failed for " + ps[i].id);
    }
    r.turnaround /= ps.size(); r.weighted /= ps.size();
    r.waiting /= ps.size(); r.response /= ps.size();
    return r;
}

vector<Process> load(const string& path) {
    ifstream f(path);
    if (!f) throw runtime_error("Cannot open " + path);
    vector<Process> ps; string line;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        istringstream in(line); Process p; int b;
        if (!(in >> p.id >> p.arrival >> p.priority)) throw runtime_error("Bad input line");
        while (in >> b) p.bursts.push_back(b);
        if (!in.eof()) throw runtime_error("Non-integer burst");
        ps.push_back(p);
    }
    validate(ps); return ps;
}
vector<Process> generate(uint32_t seed, int n) {
    if (n < 1 || n > 100) throw runtime_error("Random count must be 1..100");
    mt19937 gen(seed);
    // Modulo mapping makes the fixture reproducible across standard libraries.
    auto draw = [&](int lo, int hi) { return lo + static_cast<int>(gen() % (hi - lo + 1)); };
    vector<Process> ps;
    for (int i = 0; i < n; ++i) {
        Process p{"P" + to_string(i+1), draw(0,8), draw(1,5), {}};
        int count = draw(1,3);
        for (int j = 0; j < count; ++j) {
            p.bursts.push_back(draw(1,6));
            if (j + 1 < count) p.bursts.push_back(draw(1,5));
        }
        ps.push_back(p);
    }
    return ps;
}
void printResult(const vector<Process>& ps, const string& alg, const Result& r, bool detail) {
    if (detail) {
        cout << "\n" << alg << "\nPID  ARR CPU FIRST FIN TURN WAIT RESP WEIGHT\n";
        for (size_t i=0; i<ps.size(); ++i) {
            const auto& p=r.pcb[i]; int turn=p.finish-ps[i].arrival;
            cout << left << setw(5) << ps[i].id << right << setw(3) << ps[i].arrival
                 << setw(4) << cpuTotal(ps[i]) << setw(6) << p.first << setw(4) << p.finish
                 << setw(5) << turn << setw(5) << p.waiting << setw(5) << p.first-ps[i].arrival
                 << setw(7) << fixed << setprecision(2) << double(turn)/cpuTotal(ps[i]) << '\n';
        }
        cout << "GANTT ";
        for (size_t i=0; i<r.timeline.size();) {
            size_t j=i+1; while (j<r.timeline.size() && r.timeline[j]==r.timeline[i]) ++j;
            cout << i << '-' << j << ':' << r.timeline[i] << ' '; i=j;
        }
        cout << '\n';
    }
    cout << left << setw(10) << alg << right << fixed << setprecision(3)
         << setw(9) << r.turnaround << setw(10) << r.weighted << setw(9) << r.waiting
         << setw(9) << r.response << setw(9) << 100.0*r.busy/r.elapsed << setw(8) << r.dispatches << '\n';
}
void require(bool ok, const string& msg) { if (!ok) throw runtime_error("TEST FAILED: " + msg); }
void selfTest() {
    vector<Process> basic={{"P1",0,3,{8}},{"P2",1,2,{4}},{"P3",2,1,{1}},{"P4",3,0,{5}}};
    vector<vector<int>> expected={{8,12,13,18},{8,13,9,18},{8,18,14,13},{17,11,5,18}};
    for(size_t a=0;a<algorithms.size();++a) {
        auto r=simulate(basic,algorithms[a],2);
        for(size_t i=0;i<basic.size();++i) require(r.pcb[i].finish==expected[a][i],algorithms[a]+" completion");
        cout << "PASS " << algorithms[a] << " hand-calculated completion times\n";
    }
    vector<Process> io={{"A",2,1,{2,3,1}},{"B",3,1,{2}}};
    auto r=simulate(io,"FCFS",2);
    require(r.pcb[0].finish==8 && r.pcb[1].finish==6 && r.busy==5 && r.elapsed==8,"I/O idle");
    cout << "PASS I/O blocking, wakeup and idle intervals\n";
    for(auto alg:algorithms) {
        auto single=simulate({{"A",0,0,{1,2,1}}},alg,1);
        require(single.pcb[0].finish==4 && single.pcb[0].waiting==0,"single");
        auto tie=simulate({{"A",0,1,{1}},{"B",0,1,{1}}},alg,1);
        require(tie.pcb[0].finish==1 && tie.pcb[1].finish==2,"tie order");
    }
    cout << "PASS single process and simultaneous ties\n";
    auto boundary=simulate({{"A",0,1,{3}},{"B",2,1,{1}}},"RR",2);
    require(boundary.pcb[1].finish==3 && boundary.pcb[0].finish==4,"arrival before RR requeue");
    auto burst=simulate({{"A",0,1,{1,2,1}},{"B",0,1,{4}},{"C",3,1,{2}}},"SJF",2);
    require(burst.pcb[0].finish==6 && burst.pcb[2].finish==8,"SJF next burst");
    cout << "PASS RR boundary arrival and SJF next CPU burst selection\n";
    int rejected=0;
    for(auto bad:vector<vector<Process>>{{},{{"A",-1,1,{1}}},{{"A",0,1,{0}}},{{"A",0,1,{1,2}}},{{"A",0,1,{1}},{"A",1,1,{1}}}}) {
        try { simulate(bad,"RR",2); } catch(const exception&) { ++rejected; }
    }
    try { simulate(basic,"RR",0); } catch(const exception&) { ++rejected; }
    require(rejected==6,"input rejection");
    cout << "PASS six invalid input cases\n";
    for(uint32_t s=0;s<100;++s) for(auto alg:algorithms) {
        auto ps=generate(s,8); auto out=simulate(ps,alg,2);
        int total=0; for(auto p:ps) total+=cpuTotal(p);
        require(out.busy==total,"CPU conservation");
        for(size_t i=0;i<ps.size();++i) require(out.pcb[i].finish>=ps[i].arrival && out.pcb[i].waiting>=0,"state bounds");
    }
    cout << "PASS 400 random algorithm runs with CPU and turnaround conservation\nALL TESTS PASSED\n";
}
int integer(const string& s) {
    size_t pos; long long x=stoll(s,&pos);
    if(pos!=s.size() || x<0 || x>2147483647) throw runtime_error("Invalid numeric option: "+s);
    return static_cast<int>(x);
}
int main(int argc,char** argv) {
    try {
        string file, alg="ALL"; uint32_t seed=20261108; int n=6,q=2;
        bool random=false,trace=false,summary=false;
        for(int i=1;i<argc;++i) {
            string a=argv[i];
            auto value=[&](){if(i+1>=argc)throw runtime_error("Missing value for "+a);return string(argv[++i]);};
            if(a=="--test") {selfTest();return 0;}
            if(a=="--file")file=value();
            else if(a=="--seed")seed=static_cast<uint32_t>(integer(value()));
            else if(a=="--count")n=integer(value());
            else if(a=="--quantum")q=integer(value());
            else if(a=="--algorithm")alg=value();
            else if(a=="--random")random=true;
            else if(a=="--trace")trace=true;
            else if(a=="--summary")summary=true;
            else throw runtime_error("Unknown option: "+a);
        }
        if(!random && file.empty())file="data/basic.txt";
        auto ps=random?generate(seed,n):load(file);
        cout << "CPU scheduling simulator | C++17 | single CPU\n"
             << "Name: 王旭升  Student ID: 24281204\n"
             << "Workload: " << (random?"random seed="+to_string(seed):file) << " quantum=" << q << '\n';
        for(auto p:ps) {cout<<p.id<<" arrival="<<p.arrival<<" priority="<<p.priority<<" bursts=";
            for(size_t k=0;k<p.bursts.size();++k)cout<<(k?"/":"")<<p.bursts[k];cout<<'\n';}
        cout<<"Algorithm  AvgTurn AvgWeight  AvgWait  AvgResp CPU_pct Dispatch\n";
        if(alg!="ALL" && find(algorithms.begin(),algorithms.end(),alg)==algorithms.end())throw runtime_error("Unknown algorithm");
        for(auto a:algorithms)if(alg=="ALL"||alg==a){auto r=simulate(ps,a,q,trace?&cout:nullptr);printResult(ps,a,r,!summary);}
        return 0;
    }catch(const exception& e){cerr<<"ERROR: "<<e.what()<<'\n';return 1;}
}
