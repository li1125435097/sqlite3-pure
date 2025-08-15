#include <node.h>
#include <node_buffer.h>
#include <assert.h>
#include <sqlite3.h>
#include <uv.h>
#include <vector>
#include <string>
#include <sstream>

using namespace v8;

sqlite3* db = nullptr;  // Global database handle

// Structure to hold query results
struct QueryResult {
    std::vector<std::string> colNames;
    std::vector<int> colTypes; // Store SQLite column types
    std::vector<std::vector<std::string>> rows; // String representations
    std::vector<std::vector<std::pair<bool, std::string>>> blobs; // BLOB data
    std::string errorMsg;
    std::string debugMsg; // For debug info
    int rc;
};

// Structure for async work
struct AsyncQueryBaton {
    uv_work_t request;
    Persistent<Promise::Resolver> resolver;
    std::string query;
    QueryResult result;
    Isolate* isolate;
};

void OpenDb(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    HandleScope scope(isolate);

    assert(args[0]->IsString());  // Database path

    if (db != nullptr) {
        args.GetReturnValue().Set(Integer::New(isolate, 0));
    } else {
        String::Utf8Value dbPath(isolate, args[0].As<String>());
        int rc = sqlite3_open(*dbPath, &db);
        args.GetReturnValue().Set(Integer::New(isolate, rc));
    }
}

// Work function executed in the thread pool
void ExecuteQuery(uv_work_t* req) {
    AsyncQueryBaton* baton = static_cast<AsyncQueryBaton*>(req->data);
    std::stringstream debug;
    const char* sql = baton->query.c_str();
    const char* tail = nullptr;

    if (!db) {
        baton->result.errorMsg = "Database not initialized";
        baton->result.rc = SQLITE_ERROR;
        debug << "Error: Database not initialized\n";
        baton->result.debugMsg = debug.str();
        return;
    }

    while (*sql) {
        sqlite3_stmt* stmt;
        debug << "Preparing statement: " << sql << "\n";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, &tail) != SQLITE_OK) {
            baton->result.errorMsg = sqlite3_errmsg(db);
            baton->result.rc = SQLITE_ERROR;
            debug << "Prepare failed: " << baton->result.errorMsg << "\n";
            baton->result.debugMsg = debug.str();
            return;
        }

        int colCount = sqlite3_column_count(stmt);
        debug << "Column count: " << colCount << "\n";
        if (colCount > 0 && baton->result.colNames.empty()) {
            for (int i = 0; i < colCount; i++) {
                const char* colName = sqlite3_column_name(stmt, i);
                baton->result.colNames.push_back(colName ? colName : "");
                debug << "Column " << i << ": " << (colName ? colName : "null") << "\n";
            }
        }

        int rowCount = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::vector<std::string> row;
            std::vector<std::pair<bool, std::string>> blobRow;
            if (colCount > 0 && baton->result.colTypes.empty()) {
                for (int i = 0; i < colCount; i++) {
                    baton->result.colTypes.push_back(sqlite3_column_type(stmt, i));
                    debug << "Column " << i << " type: " << sqlite3_column_type(stmt, i) << "\n";
                }
            }
            for (int i = 0; i < colCount; i++) {
                int type = sqlite3_column_type(stmt, i);
                if (type == SQLITE_NULL) {
                    row.push_back("NULL");
                    blobRow.push_back({false, ""});
                } else if (type == SQLITE_BLOB) {
                    const void* blob = sqlite3_column_blob(stmt, i);
                    int blobSize = sqlite3_column_bytes(stmt, i);
                    row.push_back("");
                    blobRow.push_back({true, std::string(static_cast<const char*>(blob), blobSize)});
                } else {
                    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                    row.push_back(text ? text : "");
                    blobRow.push_back({false, ""});
                }
            }
            if (colCount > 0) {
                baton->result.rows.push_back(row);
                baton->result.blobs.push_back(blobRow);
            }
            rowCount++;
        }
        debug << "Rows fetched: " << rowCount << "\n";

        baton->result.rc = sqlite3_finalize(stmt);
        debug << "Finalize result: " << baton->result.rc << "\n";
        if (baton->result.rc != SQLITE_OK) {
            baton->result.errorMsg = sqlite3_errmsg(db);
            baton->result.debugMsg = debug.str();
            return;
        }

        sql = tail; // Move to next statement
        while (*tail && isspace(*tail)) tail++; // Skip whitespace
    }
    baton->result.debugMsg = debug.str();
}

// Callback function executed after work is complete
void AfterExecuteQuery(uv_work_t* req, int status) {
    Isolate* isolate = static_cast<AsyncQueryBaton*>(req->data)->isolate;
    HandleScope scope(isolate);
    AsyncQueryBaton* baton = static_cast<AsyncQueryBaton*>(req->data);
    
    Local<Context> context = isolate->GetCurrentContext();
    Local<Promise::Resolver> resolver = Local<Promise::Resolver>::New(isolate, baton->resolver);
    
    if (baton->result.rc != SQLITE_OK) {
        // Reject promise with error
        Local<String> error = String::NewFromUtf8(isolate, baton->result.errorMsg.c_str()).ToLocalChecked();
        resolver->Reject(context, Exception::Error(error)).Check();
    } else {
        // Resolve promise with results
        Local<Array> jsResult = Array::New(isolate, baton->result.rows.size());
        for (size_t i = 0; i < baton->result.rows.size(); i++) {
            Local<Object> rowObj = Object::New(isolate);
            for (size_t j = 0; j < baton->result.colNames.size(); j++) {
                Local<String> key = String::NewFromUtf8(isolate, baton->result.colNames[j].c_str()).ToLocalChecked();
                Local<Value> value;
                int type = baton->result.colTypes[j];
                const std::string& rawValue = baton->result.rows[i][j];
                
                if (type == SQLITE_NULL) {
                    value = Null(isolate);
                } else if (type == SQLITE_INTEGER) {
                    try {
                        int64_t intValue = std::stoll(rawValue);
                        if (intValue >= -2147483648LL && intValue <= 2147483647LL) {
                            value = Integer::New(isolate, static_cast<int32_t>(intValue));
                        } else {
                            value = Number::New(isolate, static_cast<double>(intValue));
                        }
                    } catch (...) {
                        value = String::NewFromUtf8(isolate, rawValue.c_str()).ToLocalChecked();
                    }
                } else if (type == SQLITE_FLOAT) {
                    try {
                        double doubleValue = std::stod(rawValue);
                        value = Number::New(isolate, doubleValue);
                    } catch (...) {
                        value = String::NewFromUtf8(isolate, rawValue.c_str()).ToLocalChecked();
                    }
                } else if (type == SQLITE_TEXT) {
                    value = String::NewFromUtf8(isolate, rawValue.c_str()).ToLocalChecked();
                } else if (type == SQLITE_BLOB) {
                    const auto& blobData = baton->result.blobs[i][j];
                    if (blobData.first) {
                        value = node::Buffer::Copy(isolate, blobData.second.data(), blobData.second.size()).ToLocalChecked();
                    } else {
                        value = String::NewFromUtf8(isolate, "[BLOB]").ToLocalChecked();
                    }
                }
                rowObj->Set(context, key, value).Check();
            }
            jsResult->Set(context, i, rowObj).Check();
        }
        resolver->Resolve(context, jsResult).Check();
    }
    
    // Clean up
    baton->resolver.Reset();
    delete baton;
}

void Exec(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    HandleScope scope(isolate);

    assert(args[0]->IsString());  // SQL query
    
    // Create a Promise resolver
    Local<Promise::Resolver> resolver = Promise::Resolver::New(context).ToLocalChecked();
    
    AsyncQueryBaton* baton = new AsyncQueryBaton();
    baton->request.data = baton;
    baton->isolate = isolate;
    
    String::Utf8Value querytxt(isolate, args[0].As<String>());
    baton->query = std::string(*querytxt);
    
    baton->resolver.Reset(isolate, resolver);
    
    // Queue the work
    uv_queue_work(uv_default_loop(), &baton->request, ExecuteQuery, AfterExecuteQuery);
    
    args.GetReturnValue().Set(resolver->GetPromise());
}

void CloseDb(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    int rc = sqlite3_close(db);
    db = nullptr;
    args.GetReturnValue().Set(Integer::New(isolate, rc));
}

void Initialize(Local<Object> exports) {
    NODE_SET_METHOD(exports, "OpenDb", OpenDb);
    NODE_SET_METHOD(exports, "Exec", Exec);
    NODE_SET_METHOD(exports, "CloseDb", CloseDb);
}

NODE_MODULE(js_executor, Initialize)