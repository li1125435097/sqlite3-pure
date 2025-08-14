#include <node.h>
#include <node_buffer.h>
#include <assert.h>
#include <sqlite3.h>
#include <vector>
#include <string>

using namespace v8;

sqlite3* db;  // Global database handle

// Structure to hold query results for passing to JavaScript
struct QueryResult {
    std::vector<std::string> colNames;
    std::vector<std::vector<std::string>> rows;
};

// Callback function to collect query results
static int callback(void* data, int argc, char** argv, char** colName) {
    QueryResult* result = static_cast<QueryResult*>(data);
    
    // Store column names on first call
    if (result->colNames.empty()) {
        for (int i = 0; i < argc; i++) {
            result->colNames.push_back(colName[i] ? colName[i] : "");
        }
    }
    
    // Store row data
    std::vector<std::string> row;
    for (int i = 0; i < argc; i++) {
        row.push_back(argv[i] ? argv[i] : "NULL");
    }
    result->rows.push_back(row);
    
    return 0;
}

void OpenDb(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    HandleScope scope(isolate);

    assert(args[0]->IsString());  // Database path
    
    String::Utf8Value dbPath(isolate, args[0].As<String>());
    int rc = sqlite3_open(*dbPath, &db);
    
    args.GetReturnValue().Set(Integer::New(isolate, rc));
}

void Exec(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    HandleScope scope(isolate);

    assert(args[0]->IsString());  // SQL query
    
    String::Utf8Value querytxt(isolate, args[0].As<String>());
    const char* querySQL = *querytxt;
    
    QueryResult result;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, querySQL, callback, &result, &errMsg);
    
    if (rc != SQLITE_OK) {
        // Return error message if query fails
        Local<String> error = String::NewFromUtf8(isolate, errMsg ? errMsg : "Unknown error").ToLocalChecked();
        sqlite3_free(errMsg);
        isolate->ThrowException(Exception::Error(error));
        return;
    }
    
    // Convert query results to JavaScript array of objects
    Local<Array> jsResult = Array::New(isolate, result.rows.size());
    for (size_t i = 0; i < result.rows.size(); i++) {
        Local<Object> rowObj = Object::New(isolate);
        for (size_t j = 0; j < result.colNames.size(); j++) {
            Local<String> key = String::NewFromUtf8(isolate, result.colNames[j].c_str()).ToLocalChecked();
            Local<String> value = String::NewFromUtf8(isolate, result.rows[i][j].c_str()).ToLocalChecked();
            rowObj->Set(context, key, value).Check();
        }
        jsResult->Set(context, i, rowObj).Check();
    }
    
    args.GetReturnValue().Set(jsResult);
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "OpenDb", OpenDb);
    NODE_SET_METHOD(exports, "Exec", Exec);
}

NODE_MODULE(js_executor, Initialize)