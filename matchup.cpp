#include "matchup.hpp"
#include <stdlib.h>
#include <iostream>

using namespace v8;
using namespace node;
using namespace std;

// This is the function called directly from JavaScript land. It creates a
// work request object and schedules it for execution.
Handle<Value> Evaluate(const Arguments& args) {
    HandleScope scope;

    if (!Buffer::HasInstance(args[0])) {
      return ThrowException(Exception::Error(
                  String::New("First argument needs to be a buffer")));
    }

    if (!Buffer::HasInstance(args[1])) {
      return ThrowException(Exception::Error(
                  String::New("Second argument needs to be a buffer")));
    }

    if (!args[2]->IsFunction()) {
        return ThrowException(Exception::TypeError(
            String::New("Third argument must be a callback function")));
    }

    Local<Object> candidates = args[0]->ToObject();
    Local<Object> answers = args[1]->ToObject();

    // There's no ToFunction(), use a Cast instead.
    Local<Function> callback = Local<Function>::Cast(args[2]);

    // The baton holds our custom status information for this asynchronous call,
    // like the callback function we want to call when returning to the main
    // thread and the status information.
    Baton* baton = new Baton();
    baton->error = false;
    baton->callback = Persistent<Function>::New(callback);
    baton->candidates = Buffer::Data(candidates);
    baton->answers = Buffer::Data(answers);
    baton->answerCount = Buffer::Length(answers);

    // This creates the work request struct.
    uv_work_t *req = new uv_work_t();
    req->data = baton;

    // Schedule our work request with libuv. Here you can specify the functions
    // that should be executed in the threadpool and back in the main thread
    // after the threadpool function completed.
    int status = uv_queue_work(uv_default_loop(), req, AsyncWork,
                               (uv_after_work_cb)AsyncAfter);
    assert(status == 0);

    return Undefined();
}

// This function is executed in another thread at some point after it has been
// scheduled. IT MUST NOT USE ANY V8 FUNCTIONALITY. Otherwise your extension
// will crash randomly and you'll have a lot of fun debugging.
// If you want to use parameters passed into the original call, you have to
// convert them to PODs or some other fancy method.
void AsyncWork(uv_work_t* req) {
    Baton* baton = static_cast<Baton*>(req->data);

    ProcessData(baton);
}

// This function is executed in the main V8/JavaScript thread. That means it's
// safe to use V8 functions again. Don't forget the HandleScope!
void AsyncAfter(uv_work_t* req) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);

    if (baton->error) {
        Local<Value> err = Exception::Error(String::New(baton->error_message.c_str()));

        // Prepare the parameters for the callback function.
        const unsigned argc = 1;
        Local<Value> argv[argc] = { err };

        // Wrap the callback function call in a TryCatch so that we can call
        // node's FatalException afterwards. This makes it possible to catch
        // the exception from JavaScript land using the
        // process.on('uncaughtException') event.
        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught()) {
            node::FatalException(try_catch);
        }
    } else {
        Buffer *resultBuffer = CreateResultBuffer(baton);

        // In case the operation succeeded, convention is to pass null as the
        // first argument before the result arguments.
        // In case you produced more complex data, this is the place to convert
        // your plain C++ data structures into JavaScript/V8 data structures.
        const unsigned argc = 2;
        Local<Value> argv[argc] = {
            Local<Value>::New(Null()),
            Local<Value>::New(resultBuffer->handle_)
        };

        // Wrap the callback function call in a TryCatch so that we can call
        // node's FatalException afterwards. This makes it possible to catch
        // the exception from JavaScript land using the
        // process.on('uncaughtException') event.
        TryCatch try_catch;
        baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
        if (try_catch.HasCaught()) {
            node::FatalException(try_catch);
        }
    }

    // The callback is a permanent handle, so we have to dispose of it manually.
    baton->callback.Dispose();

    // We also created the baton and the work_req struct with new, so we have to
    // manually delete both.
    delete baton;
    delete req;
}

// This is the function called directly from JavaScript land. It creates a
// work request object and schedules it for execution.
Handle<Value> EvaluateSync(const Arguments& args) {
    HandleScope scope;

    if (!Buffer::HasInstance(args[0])) {
      return ThrowException(Exception::Error(
                  String::New("First argument needs to be a buffer")));
    }

    if (!Buffer::HasInstance(args[1])) {
      return ThrowException(Exception::Error(
                  String::New("Second argument needs to be a buffer")));
    }

    Local<Object> candidates = args[0]->ToObject();
    Local<Object> answers = args[1]->ToObject();

    // The baton holds our custom status information for this asynchronous call,
    // like the callback function we want to call when returning to the main
    // thread and the status information.
    Baton* baton = new Baton();
    baton->error = false;
    baton->candidates = Buffer::Data(candidates);
    baton->answers = Buffer::Data(answers);
    baton->answerCount = Buffer::Length(answers);

    ProcessData(baton);
    Buffer *resultBuffer = CreateResultBuffer(baton);

    delete baton;

    return scope.Close(Local<Value>::New(resultBuffer->handle_));
}

void ProcessData(Baton* baton) {
    CandidateScore *first = NULL;
    CandidateScore *last = NULL;
    CandidateScore *score, *tmp;
    char *cursor = baton->candidates;
    uint16_t candidateCount = *(uint16_t*)cursor;
    uint16_t answerCount = *(uint16_t*)(cursor+2);
    cursor += 4;

    for (uint i = 0; i < candidateCount; i++) {
        score = new CandidateScore();
        score->id = *((uint32_t*)cursor);
        score->score = 0;
        score->next = NULL;
        score->previous = NULL;
        cursor += 4;

        // Calculate the match score.
        for (uint j = 0; j < answerCount; j++) {
            score->score += abs(cursor[j] - baton->answers[j]);
            // Abort evaluation if i>10 and score > last->score
            if (i > 10 && score->score > last->score) {
                delete score;
                score = NULL;
                break;
            }
        }
        cursor += answerCount;

        // Abort if we decided to discard the candidate
        if (!score) {
            continue;
        }

        // Slot the result into our linked list of results
        if (!first) {
            first = score;
            last = score;
        }
        else {
            tmp = last;
            // Walk from the end to the beginning to find an item with a lower
            // score that we can add our result after.
            do {
                // If we have a score lower than the current item
                if (score->score <= tmp->score) {
                    // We walk up if we're not at the beginning of the list
                    if (tmp->previous) {
                        tmp = tmp->previous;
                    }
                    else {
                        // Otherwise we add the result to the beginning of the
                        // list.
                        first = score;
                        first->next = tmp;
                        tmp->previous = first;
                        break;
                    }
                }
                else {
                    // Don't add new items at the end of the list if we already
                    // have 10 items.
                    if (i >= 10 && !tmp->next) {
                        delete score;
                        score = NULL;
                    }
                    else {
                        // Insert the result after the item with a lower score.
                        score->next = tmp->next;
                        score->previous = tmp;
                        if (score->next) {
                            score->next->previous = score;
                        }
                        tmp->next = score;
                        if (!score->next) {
                            last = score;
                        }
                    }
                    break;
                }
            } while(tmp);
        }

        // Pop off the last item if the list is getting crowded.
        if (score && (score->next || score->previous) && i >= 10) {
            tmp = last->previous;
            delete last;
            last = tmp;
            last->next = NULL;
        }
    }

    baton->result = first;
}

Buffer* CreateResultBuffer(Baton* baton) {
    char *cursor = baton->candidates;
    uint16_t candidateCount = *(uint16_t*)cursor;

    size_t resultLength = (10 < candidateCount ? 10 : candidateCount);
    Buffer *resultBuffer = Buffer::New(resultLength * 8);
    uint32_t *result = (uint32_t *)Buffer::Data(resultBuffer);

    // Put our results into the result buffer
    CandidateScore *current = baton->result, *tmpScoreRef;
    uint i = 0;
    while (current && i < resultLength*2) {
        result[i++] = current->id;
        result[i++] = current->score;

        tmpScoreRef = current;
        current = current->next;
        delete tmpScoreRef;
    }
    baton->result = NULL;
    return resultBuffer;
}

void RegisterModule(Handle<Object> target) {
    target->Set(String::NewSymbol("evaluate"),
        FunctionTemplate::New(Evaluate)->GetFunction());
    target->Set(String::NewSymbol("evaluateSync"),
        FunctionTemplate::New(EvaluateSync)->GetFunction());
}

NODE_MODULE(matchup, RegisterModule);
