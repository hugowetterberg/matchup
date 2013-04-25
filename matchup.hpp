#include <node.h>
#include <node_buffer.h>
#include <string>

struct CandidateScore {
    uint32_t id;
    uint32_t score;
    CandidateScore *previous;
    CandidateScore *next;
};

// We use a struct to store information about the asynchronous "work request".
struct Baton {
    // This handle holds the callback function we'll call after the work request
    // has been completed in a threadpool thread. It's persistent so that V8
    // doesn't garbage collect it away while our request waits to be processed.
    // This means that we'll have to dispose of it later ourselves.
    v8::Persistent<v8::Function> callback;

    // Tracking errors that happened in the worker function. You can use any
    // variables you want. E.g. in some cases, it might be useful to report
    // an error number.
    bool error;
    std::string error_message;

    // Job payload
    char  *candidates;
    size_t answerCount;
    char  *answers;
    CandidateScore *result;
};

// Forward declaration. Usually, you do this in a header file.
v8::Handle<v8::Value> Evaluate(const v8::Arguments& args);
v8::Handle<v8::Value> EvaluateSync(const v8::Arguments& args);
void AsyncWork(uv_work_t* req);
void AsyncAfter(uv_work_t* req);
void ProcessData(Baton *baton);
node::Buffer* CreateResultBuffer(Baton* baton);
