#include <bits/stdc++.h>
using namespace std;

enum class State {
    NOT_ARRIVED,
    READY_P_PRE,
    RUNNING_P_PRE,
    WAIT_P_UP,
    READY_P_PROC,
    RUNNING_P_PROC,
    WAIT_P_DOWN,
    READY_P_POST,
    RUNNING_P_POST,
    READY_D_PRE,
    RUNNING_D_PRE,
    WAIT_D_UP,
    READY_D_PROC,
    RUNNING_D_PROC,
    WAIT_D_DOWN,
    READY_D_POST,
    RUNNING_D_POST,
    FINISHED
};

struct Request {
    State state = State::NOT_ARRIVED;
    int inputLength = 0;
    int remote = -1;
    int nextLayer = 0;
    long long readyFrame = 0;
};

struct Assignment {
    string line;
};

static vector<string> split(const string& line) {
    istringstream in(line);
    vector<string> tokens;
    string token;
    while (in >> token) tokens.push_back(token);
    return tokens;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int K, bytesPerToken, numLayers;
    double scheduleCost, latencyMs, bandwidthGbps;
    if (!(cin >> K >> scheduleCost >> latencyMs >> bandwidthGbps
              >> bytesPerToken >> numLayers)) {
        return 0;
    }

    double SLO1, SLO2, tpUB, tpBase, distBase, wTp, wC;
    cin >> SLO1 >> SLO2 >> tpUB >> tpBase >> distBase >> wTp >> wC;

    int N;
    cin >> N;
    for (int i = 0; i < N; ++i) {
        int batchSize;
        double prefillPre, prefillProc, prefillPost;
        double decodePre, decodeProc, decodePost;
        cin >> batchSize >> prefillPre >> prefillProc >> prefillPost
            >> decodePre >> decodeProc >> decodePost;
    }

    string line;
    getline(cin, line);

    vector<Request> requests;
    vector<int> activeOnRemote(K, 0);
    vector<char> remoteBusy(K, false);
    bool localBusy = false;
    long long frameNumber = 0;

    auto ensureRequest = [&](int rid) {
        if (rid >= (int)requests.size()) requests.resize(rid + 1);
    };

    auto makeReady = [&](int rid, State state) {
        ensureRequest(rid);
        if (requests[rid].state == State::FINISHED) return;
        requests[rid].state = state;
        requests[rid].readyFrame = frameNumber;
    };

    while (getline(cin, line)) {
        if (line.empty()) continue;
        if (line == "END") return 0;

        ++frameNumber;

        string eventCountLine;
        if (!getline(cin, eventCountLine)) return 0;
        int eventCount = stoi(eventCountLine);

        vector<vector<string>> events;
        events.reserve(eventCount);
        for (int i = 0; i < eventCount; ++i) {
            string eventLine;
            if (!getline(cin, eventLine)) return 0;
            events.push_back(split(eventLine));
        }

        for (const auto& event : events) {
            if (event.empty()) continue;

            if (event[0] == "ARR") {
                int rid = stoi(event[1]);
                int inputLength = stoi(event[2]);
                ensureRequest(rid);
                requests[rid].inputLength = inputLength;
                requests[rid].remote = -1;
                requests[rid].nextLayer = 0;
                makeReady(rid, State::READY_P_PRE);
                continue;
            }

            if (event[0] == "FIN") {
                int rid = stoi(event[1]);
                ensureRequest(rid);
                if (requests[rid].state != State::FINISHED) {
                    int remote = requests[rid].remote;
                    if (0 <= remote && remote < K) --activeOnRemote[remote];
                    requests[rid].state = State::FINISHED;
                }
                continue;
            }

            if (event[0] == "XDN") {
                const string& direction = event[1];
                const string& stage = event[4];
                int memberCount = stoi(event[5]);
                for (int j = 0; j < memberCount; ++j) {
                    int rid = stoi(event[6 + j]);
                    if (stage == "PRE") {
                        if (direction == "UP") {
                            makeReady(rid, State::READY_P_PROC);
                        } else {
                            makeReady(rid, State::READY_P_POST);
                        }
                    } else {
                        if (direction == "UP") {
                            makeReady(rid, State::READY_D_PROC);
                        } else {
                            makeReady(rid, State::READY_D_POST);
                        }
                    }
                }
                continue;
            }

            if (event[0] == "TDN") {
                const string& server = event[1];
                if (server == "E") {
                    localBusy = false;
                } else {
                    int remote = stoi(server.substr(1));
                    remoteBusy[remote] = false;
                }

                const string& stage = event[2];
                const string& step = event[3];

                if (stage == "P") {
                    if (step == "PRE") {
                        int rid = stoi(event[5]);
                        makeReady(rid, State::WAIT_P_UP);
                    } else if (step == "PROC") {
                        int layerEnd = stoi(event[5]);
                        int rid = stoi(event[7]);
                        requests[rid].nextLayer = layerEnd;
                        if (layerEnd == numLayers) {
                            makeReady(rid, State::WAIT_P_DOWN);
                        } else {
                            makeReady(rid, State::READY_P_PROC);
                        }
                    } else {
                        int rid = stoi(event[5]);
                        makeReady(rid, State::READY_D_PRE);
                    }
                } else {
                    int memberCount = stoi(event[5]);
                    for (int j = 0; j < memberCount; ++j) {
                        int rid = stoi(event[6 + j]);
                        if (step == "PRE") {
                            makeReady(rid, State::WAIT_D_UP);
                        } else if (step == "PROC") {
                            makeReady(rid, State::WAIT_D_DOWN);
                        } else {
                            makeReady(rid, State::READY_D_PRE);
                        }
                    }
                }
            }
        }

        vector<Assignment> assignments;

        for (int remote = 0; remote < K; ++remote) {
            if (remoteBusy[remote]) continue;

            int chosen = -1;
            for (int rid = 0; rid < (int)requests.size(); ++rid) {
                const Request& request = requests[rid];
                if (request.remote != remote) continue;
                if (request.state != State::READY_P_PROC &&
                    request.state != State::READY_D_PROC) {
                    continue;
                }
                if (chosen == -1 ||
                    tie(request.readyFrame, rid) <
                        tie(requests[chosen].readyFrame, chosen)) {
                    chosen = rid;
                }
            }

            if (chosen == -1) continue;

            Request& request = requests[chosen];
            if (request.state == State::READY_P_PROC) {
                assignments.push_back({
                    "C" + to_string(remote) + " P PROC " +
                    to_string(request.nextLayer) + " " + to_string(numLayers) +
                    " " + to_string(remote) + " " + to_string(chosen)
                });
                request.state = State::RUNNING_P_PROC;
            } else {
                assignments.push_back({
                    "C" + to_string(remote) + " D PROC " +
                    to_string(remote) + " 1 " + to_string(chosen)
                });
                request.state = State::RUNNING_D_PROC;
            }
            remoteBusy[remote] = true;
        }

        if (!localBusy) {
            int chosen = -1;
            for (int rid = 0; rid < (int)requests.size(); ++rid) {
                State state = requests[rid].state;
                bool locallyReady =
                    state == State::READY_P_PRE ||
                    state == State::READY_P_POST ||
                    state == State::READY_D_PRE ||
                    state == State::READY_D_POST;
                if (!locallyReady) continue;

                if (chosen == -1 ||
                    tie(requests[rid].readyFrame, rid) <
                        tie(requests[chosen].readyFrame, chosen)) {
                    chosen = rid;
                }
            }

            if (chosen != -1) {
                Request& request = requests[chosen];
                if (request.state == State::READY_P_PRE) {
                    int remote = static_cast<int>(
                        min_element(activeOnRemote.begin(),
                                    activeOnRemote.end()) -
                        activeOnRemote.begin()
                    );
                    request.remote = remote;
                    ++activeOnRemote[remote];
                    assignments.push_back({
                        "E P PRE " + to_string(remote) + " " +
                        to_string(chosen)
                    });
                    request.state = State::RUNNING_P_PRE;
                } else if (request.state == State::READY_P_POST) {
                    assignments.push_back({
                        "E P POST " + to_string(request.remote) + " " +
                        to_string(chosen)
                    });
                    request.state = State::RUNNING_P_POST;
                } else if (request.state == State::READY_D_PRE) {
                    assignments.push_back({
                        "E D PRE -1 1 " + to_string(chosen)
                    });
                    request.state = State::RUNNING_D_PRE;
                } else {
                    assignments.push_back({
                        "E D POST -1 1 " + to_string(chosen)
                    });
                    request.state = State::RUNNING_D_POST;
                }
                localBusy = true;
            }
        }

        cout << assignments.size() << '\n';
        for (const Assignment& assignment : assignments) {
            cout << assignment.line << '\n';
        }
        cout << flush;
    }

    return 0;
}
