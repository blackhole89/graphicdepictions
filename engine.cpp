 /*
  * graphic depictions, a visual workbench for graphs
  *
  * Copyright (C) 2016-2018 Matvey Soloviev
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

#include "stdafx.h"
#include "engine.h"
#ifdef WIN32
#include "pthread.h"
#include "semaphore.h"
#else
#include <sys/time.h>
#endif

#include "v8/v8-profiler.h"

using namespace v8;

/* helpers */
const char *nonames[6]={"?","Graph","Node","Edge","Node Set","Edge Set"};

Local<Object> WrapNode(CSState::CSNode *n)
{
    auto nwrapped = s.e->node_templ->NewInstance();
    nwrapped->SetInternalField(0, External::New(n));
    nwrapped->SetInternalField(1, Integer::New(NO_NODE));
    return nwrapped;
}
Local<Object> WrapEdge(CSState::CSEdge *e)
{
    auto ewrapped = s.e->edge_templ->NewInstance();
    ewrapped->SetInternalField(0, External::New(e));
    ewrapped->SetInternalField(1, Integer::New(NO_EDGE));
    return ewrapped;
}
Local<Object> WrapNodeSet(std::set<CSState::CSNode*> *ns)
{
    HandleScope scope;

    Handle<Array> a = Array::New(ns->size());

    int i=0;
    for(auto &n : *ns) {
        a->Set(i++, WrapNode(n));
    }

    return scope.Close(a);
}

Local<Object> WrapEdgeSet(std::set<CSState::CSEdge*> *es)
{
    HandleScope scope;

    Handle<Array> a = Array::New(es->size());

    int i=0;
    for(auto &e : *es) {
        a->Set(i++, WrapEdge(e));
    }

    return scope.Close(a);
}

Handle<Value> ValueOfCallback(const Arguments &args)
{
    Handle<External> he = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    long long ptr = (long long)he->Value();

    return Integer::New(ptr);
}

/* (de)serialisation of native objects */
Handle<Value> NativeRenditionCallback(const Arguments &args)
{
    Handle<External> he = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    int type = args.Holder()->GetInternalField(1)->Int32Value(); 

    char buf[128];
    if(type==NO_NODE) {
        CSState::CSNode* ptr = (CSState::CSNode*)he->Value();
        if(s.e->st.nodes2id.size()) sprintf(buf,"_nat_node(%d)",s.e->st.nodes2id[ptr]);
        else sprintf(buf,"_nat_node(%lld)",(long long)ptr);
    } else if(type==NO_EDGE) {
        CSState::CSEdge* ptr = (CSState::CSEdge*)he->Value();
        if(s.e->st.edges2id.size()) sprintf(buf,"_nat_edge(%d)",s.e->st.edges2id[ptr]);
        else sprintf(buf,"_nat_edge(%lld)",(long long)ptr);
    }

    return String::New(buf);
}

class FileOutputStream : public OutputStream {
 public:
  FileOutputStream(FILE* stream) : stream_(stream) {}

  virtual int GetChunkSize() {
    return 65536;  // big chunks == faster
  }

  virtual void EndOfStream() {}

  virtual WriteResult WriteAsciiChunk(char* data, int size) {
    const size_t len = static_cast<size_t>(size);
    size_t off = 0;

    while (off < len && !feof(stream_) && !ferror(stream_))
      off += fwrite(data + off, 1, len - off, stream_);

    return off == len ? kContinue : kAbort;
  }

 private:
  FILE* stream_;
};

Handle<Value> SnapHeapCallback(const Arguments &args)
{
  FILE* fp = fopen("heap_snapshot", "w");
  const HeapSnapshot* const snap = HeapProfiler::TakeSnapshot(String::New(""));
  FileOutputStream stream(fp);
  snap->Serialize(&stream, HeapSnapshot::kJSON);
  fclose(fp);
  HeapProfiler::DeleteAllSnapshots();
}

Handle<Value> NatNodeCallback(const Arguments &args)
{
    if(args.Length()<1) return v8::Undefined();

    if(s.e->st.id2nodes.size()) return WrapNode( s.e->st.id2nodes[args[0]->Int32Value()] );
    else return WrapNode( (CSState::CSNode*)args[0]->IntegerValue() );
}
Handle<Value> NatEdgeCallback(const Arguments &args)
{
    if(args.Length()<1) return v8::Undefined();

    if(s.e->st.id2edges.size()) return WrapEdge( s.e->st.id2edges[args[0]->Int32Value()] );
    else return WrapEdge( (CSState::CSEdge*)args[0]->IntegerValue() );
}

Handle<Value> SetColourCallback(const Arguments &args)
{
    if (args.Length()<3) return v8::Undefined();

    if(!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsNumber()) return v8::Undefined();

    Handle<External> hn = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    CSState::CSNode *n = (CSState::CSNode*)hn->Value();

    double r=args[0]->NumberValue(), g=args[1]->NumberValue(), b=args[2]->NumberValue();

    /* prevent parse failures */
    if(isnan(r)||isnan(g)||isnan(b)||isinf(r)||isinf(g)||isinf(b)) return v8::Undefined();

    char buf[256];
    sprintf(buf,"[%f,%f,%f]",r,g,b);
    n->a["clr"] = CSState::CSAttr(buf);

    return v8::Undefined();
}

Local<Value> CSEngine::FromJSON(char *j)
{
    HandleScope scope;
    Handle<Value> jsonVal = s.e->st.v8ctx->Global()->Get(String::New("JSON"));
    Handle<Object> json = jsonVal->ToObject();
    Handle<Value> parseVal = json->Get(String::New("strong_parse"));
    Handle<Function> parse = Handle<Function>::Cast(parseVal); 
    
    Handle<Value> input=String::New(j);

    Handle<Value> ret= parse->Call(json, 1, &input);

//    printf("From JSON: %s becomes %s.\n",j,*String::Utf8Value(ret));

    return scope.Close(ret);
}

std::string CSEngine::ToJSON(Handle<Value> val)
{
    HandleScope scope;
    Handle<Value> jsonVal = s.e->st.v8ctx->Global()->Get(String::New("JSON"));
    Handle<Object> json = jsonVal->ToObject();
    Handle<Value> stringifyVal = json->Get(String::New("strong_stringify"));
    Handle<Function> stringify = Handle<Function>::Cast(stringifyVal);

    Handle<Value> res = stringify->Call(json, 1, &val);

    Handle<String> st = res->ToString();
    char *buf = new char[st->Utf8Length()+1];
    res->ToString()->WriteUtf8(buf);
    std::string ret(buf);
    delete [] buf;
    return ret;
}

/* set all wrapped instances of ptr to undefined */
void CSEngine::DeepWipe(void *ptr)
{
    Local<Value> jsonVal = s.e->st.v8ctx->Global()->Get(String::New("JSON"));
    Local<Object> json = jsonVal->ToObject();
    Local<Value> deepWipeVal = json->Get(String::New("deep_wipe"));
    Local<Function> deepWipe = Local<Function>::Cast(deepWipeVal);

    HandleScope scope;
    Local<Integer> p = Integer::New((long long)ptr);

    /* wipe global context */
    Local<Value> args[2] = {p,st.v8ctx->Global()};
    deepWipe->Call(json, 2, args);

    /* wipe all attribute holders */
    for(auto &n: st.nodes) {
        Local<Value> args[2] = {p,WrapNode(n)};
        deepWipe->Call(json, 2, args);
    }
    for(auto &e: st.edges) {
        Local<Value> args[2] = {p,WrapEdge(e)};
        deepWipe->Call(json, 2, args);
    }
}

void CSEngine::ImportToGlobal(Handle<Object> obj)
{
    Local<Value> jsonVal = s.e->st.v8ctx->Global()->Get(String::New("JSON"));
    Local<Object> json = jsonVal->ToObject();
    Local<Value> importObjectVal = json->Get(String::New("import_object"));
    Local<Function> importObject = Local<Function>::Cast(importObjectVal);

    HandleScope scope;

    Handle<Value> args[2] = {st.v8ctx->Global(),obj};
    importObject->Call(json, 2, args);
}

Handle<Array> NodeEnum(const AccessorInfo &info)
{
    HandleScope scope;
    Handle<External> field = Handle<External>::Cast(info.Holder()->GetInternalField(0));
    CSState::CSNode *n = (CSState::CSNode*)field->Value();

    Handle<Array> a = Array::New(n->a.size());

    int i=0;
    for(auto &&[k,v] : n->a) {
        a->Set(i++, String::New(k.c_str()));
    }

    return scope.Close(a);
}

Handle<Value> NodeGet(Local<String> name, const AccessorInfo& info)
{
    HandleScope scope;

    Handle<External> field = Handle<External>::Cast(info.Holder()->GetInternalField(0));
    CSState::CSNode *n = (CSState::CSNode*)field->Value();

    std::string dfield = *String::Utf8Value(name);

    if(dfield=="pos") {
        Handle<Object> ret=Object::New();
        ret->Set(String::New("x"), Number::New(n->pos[0]));
        ret->Set(String::New("y"), Number::New(n->pos[1]));
        ret->Set(String::New("z"), Number::New(n->pos[2]));

        return scope.Close(ret);
    } else if(dfield=="setColor" || dfield=="setColour") {
        return scope.Close(s.e->set_colour_templ->GetFunction());
    } else if(dfield=="valueOf") {
        return scope.Close(s.e->value_of_templ->GetFunction());
    } else if(dfield=="_nativeRendition") {
        return scope.Close(s.e->native_rendition_templ->GetFunction());
    } else if(dfield=="selected") {
        return scope.Close(Boolean::New(n->selected));
    } else if(n->a.count(dfield)) {
        CSState::CSAttr *a = &n->a[dfield];
        return scope.Close(Local<Value>::New(a->j_data));
    }
    return Undefined();
}

Handle<Value> NodeSet(Local<String> name, Local<Value> value, const AccessorInfo& info)
{
    Handle<External> field = Handle<External>::Cast(info.Holder()->GetInternalField(0));
    CSState::CSNode *n = (CSState::CSNode*)field->Value();

    std::string dfield = *String::Utf8Value(name);

    if(dfield=="pos") {
        // can't set pos
    } else if(dfield=="selected") {
        if(value->IsBoolean()) {
            bool b = value->BooleanValue();
            if(b && !n->selected) ++s.e->st.nselected;
            else if(!b && n->selected) --s.e->st.nselected;
            n->selected=b;
        }
    } else {
        if( value->IsUndefined() )
            n->a.erase(dfield);
        else {
            /* deep copy for sanity */
            std::string res = s.e->ToJSON(value);
    
            n->a[dfield]=CSState::CSAttr((char*)res.c_str());

//            printf("Setting. %s = %s, %s\n",dfield.c_str(),res.c_str(),*String::Utf8Value(n->a[dfield].j_data));
        }

    }

    return value;
}

Handle<Value> EdgeForeachCallback(const Arguments &args);

Handle<Array> EdgeEnum(const AccessorInfo &info)
{
    HandleScope scope;
    Handle<External> field = Handle<External>::Cast(info.Holder()->GetInternalField(0));
    CSState::CSEdge *e = (CSState::CSEdge*)field->Value();

    Handle<Array> a = Array::New(e->a.size());

    int i=0;
    for(auto &&[k,v] : e->a) {
        a->Set(i++, String::New(k.c_str()));
    }

    return scope.Close(a);
}

Handle<Value> EdgeGet(Local<String> name, const AccessorInfo& info)
{
    HandleScope scope;

    Handle<External> field = Handle<External>::Cast(info.Holder()->GetInternalField(0));
    CSState::CSEdge *e = (CSState::CSEdge*)field->Value();

    std::string dfield = *String::Utf8Value(name);

    if(dfield=="forEach") {
        return scope.Close(s.e->edge_foreach_templ->GetFunction());
    } else if(dfield=="valueOf") {
        return scope.Close(s.e->value_of_templ->GetFunction()); 
    } else if(dfield=="_nativeRendition") {
        return scope.Close(s.e->native_rendition_templ->GetFunction());
    } else if(dfield=="n1") {
        return scope.Close(WrapNode(e->n1));
    } else if(dfield=="n2") {
        return scope.Close(WrapNode(e->n2));
    } else if(e->a.count(dfield)) {
        CSState::CSAttr *a = &e->a[dfield];
        return scope.Close(Local<Value>::New(a->j_data));
    }
    return Undefined();
}

Handle<Value> EdgeSet(Local<String> name, Local<Value> value, const AccessorInfo& info)
{
    Handle<External> field = Handle<External>::Cast(info.Holder()->GetInternalField(0));
    CSState::CSEdge *e = (CSState::CSEdge*)field->Value();

    std::string dfield = *String::Utf8Value(name);

    if(dfield=="n1" || dfield=="n2") {
        // can't set pos
    } else {
        if( value->IsUndefined() )
            e->a.erase(dfield);
        else {
            std::string res = s.e->ToJSON(value);
    
            e->a[dfield]=CSState::CSAttr((char*)res.c_str());
        }
    }

    return value;
}

Handle<Value> RandomCallback(const Arguments &args)
{
    if (args.Length()==0) return Integer::New(rand());
    return v8::Undefined();
}

Handle<Value> PrintlnCallback(const Arguments &args)
{
    if (args.Length()==0 || args.Length()>1) return Undefined();
    s.e->term_results.push_back(std::string(":")+*String::Utf8Value(args[0]));

    return Undefined();
}

Handle<Value> PrintCallback(const Arguments &args)
{
    if (args.Length()==0 || args.Length()>1) return Undefined();

    if(s.e->term_results.empty()) goto print_callback_start_newline;

    {
        std::string &bck = s.e->term_results.back();
    
        if(bck.empty()) goto print_callback_start_newline;
        if(bck[0]!=':') goto print_callback_start_newline;

        bck += *String::Utf8Value(args[0]);

        return Undefined();
    }
print_callback_start_newline:
    s.e->term_results.push_back(std::string(":")+*String::Utf8Value(args[0]));    
    return Undefined();
}

Handle<Value> CountbitsCallback(const Arguments &args)
{
    if (args.Length()!=1) return v8::Undefined();
    int ret=0;
    Handle<Value> arg = args[0];
    int v = arg->Int32Value();

    for(int i=0;i<31;++i) {
        ret+=(bool)(v&(1<<i));
    }

    return Integer::New(ret);
}

Handle<Value> AddNodeCallback(const Arguments &args)
{
    if (args.Length()<2) return v8::Undefined();

    if(!args[0]->IsNumber()) return v8::Undefined();
    if(!args[1]->IsNumber()) return v8::Undefined();

    float x = args[0]->NumberValue();
    float y = args[1]->NumberValue();
    float z = 0.0f;

    if(args.Length()>=3) {
        if(!args[2]->IsNumber()) return v8::Undefined();
        z = args[2]->NumberValue();
    }

    CSState::CSNode *n=s.e->st.AddNode(x,y,z);

    return WrapNode(n);
}

Handle<Value> DelNodeCallback(const Arguments &args)
{
    if (args.Length()<1 || args.Length()>1) return v8::Undefined();

    if(!args[0]->IsObject()) return v8::Undefined();
    
    Handle<External> hn = Handle<External>::Cast(Handle<Object>::Cast(args[0])->GetInternalField(0));
    CSState::CSNode *n = (CSState::CSNode*)hn->Value();
    
    s.e->st.DelNode(n);

    return v8::Undefined();
}

Handle<Value> AddEdgeCallback(const Arguments &args)
{
    if (args.Length()<2) return v8::Undefined();

    if(!args[0]->IsObject()) return v8::Undefined();
    if(!args[1]->IsObject()) return v8::Undefined();

    Handle<External> hn1 = Handle<External>::Cast(Handle<Object>::Cast(args[0])->GetInternalField(0));
    CSState::CSNode *n1 = (CSState::CSNode*)hn1->Value();

    Handle<External> hn2 = Handle<External>::Cast(Handle<Object>::Cast(args[1])->GetInternalField(0));
    CSState::CSNode *n2 = (CSState::CSNode*)hn2->Value();

    CSState::CSEdge* e = s.e->st.AddEdge(n1,n2);

    if(e) {
        return WrapEdge(e);
    } else return v8::Undefined();
}

Handle<Value> DelEdgeCallback(const Arguments &args)
{
    if (args.Length()<1) return v8::Undefined();

    if(!args[0]->IsObject()) return v8::Undefined();
    
    if (args.Length()==1) {
       /* delEdge(E) */ 
        Handle<External> he1 = Handle<External>::Cast(Handle<Object>::Cast(args[0])->GetInternalField(0));
        CSState::CSEdge *e = (CSState::CSEdge*)he1->Value();
    
        s.e->st.DelEdge(e);

        return v8::Undefined();
    }

    /* delEdge(N,M) */
    if(!args[1]->IsObject()) return v8::Undefined();

    Handle<External> hn1 = Handle<External>::Cast(Handle<Object>::Cast(args[0])->GetInternalField(0));
    CSState::CSNode *n1 = (CSState::CSNode*)hn1->Value();

    Handle<External> hn2 = Handle<External>::Cast(Handle<Object>::Cast(args[1])->GetInternalField(0));
    CSState::CSNode *n2 = (CSState::CSNode*)hn2->Value();

    s.e->st.DelEdge(n1,n2);

    return v8::Undefined();
}

Handle<Value> GetEdgeCallback(const Arguments &args)
{
    if (args.Length()<2) return v8::Undefined();

    if(!args[0]->IsObject()) return v8::Undefined();
    if(!args[1]->IsObject()) return v8::Undefined();

    Handle<External> hn1 = Handle<External>::Cast(Handle<Object>::Cast(args[0])->GetInternalField(0));
    CSState::CSNode *n1 = (CSState::CSNode*)hn1->Value();

    Handle<External> hn2 = Handle<External>::Cast(Handle<Object>::Cast(args[1])->GetInternalField(0));
    CSState::CSNode *n2 = (CSState::CSNode*)hn2->Value();

    CSState::CSEdge* e = s.e->st.GetEdge(n1,n2);

    if(e) {
        return WrapEdge(e);
    } else return v8::Undefined();
}

Handle<Value> GammaCallback(const Arguments &args)
{
    if (args.Length()<1) return v8::Undefined();

    if(!args[0]->IsObject()) return v8::Undefined();

    Handle<External> hn1 = Handle<External>::Cast(Handle<Object>::Cast(args[0])->GetInternalField(0));
    CSState::CSNode *n1 = (CSState::CSNode*)hn1->Value();

    return WrapNodeSet(&n1->adj);
}

Handle<Value> DeltaCallback(const Arguments &args)
{
    if (args.Length()<1) return v8::Undefined();

    if(!args[0]->IsObject()) return v8::Undefined();

    Handle<External> hn1 = Handle<External>::Cast(Handle<Object>::Cast(args[0])->GetInternalField(0));
    CSState::CSNode *n1 = (CSState::CSNode*)hn1->Value();

    return WrapEdgeSet(&n1->adje);
}

Handle<Value> NodesCallback(const Arguments &args)
{
    if (args.Length()!=0) return v8::Undefined();

    return WrapNodeSet(&s.e->st.nodes);
}

Handle<Value> EdgesCallback(const Arguments &args)
{
    if (args.Length()!=0) return v8::Undefined();

    return WrapEdgeSet(&s.e->st.edges);
}

Handle<Value> SizeCallback(const Arguments &args)
{
    // hoping for a this pointer
    Handle<External> hs = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    std::set<CSState::CSNode*> *s = (std::set<CSState::CSNode*>*)hs->Value();

    return Integer::New(s->size());
}

Handle<Value> EdgeSetSizeCallback(const Arguments &args)
{
    // hoping for a this pointer
    Handle<External> hs = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    std::set<CSState::CSEdge*> *s = (std::set<CSState::CSEdge*>*)hs->Value();

    return Integer::New(s->size());
}

Handle<Value> ForeachCallback(const Arguments &args)
{
    if (args.Length()<1) return v8::Undefined();

    if(!args[0]->IsFunction()) return v8::Undefined();

    // hoping for a this pointer
    Handle<External> hs = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    // create local copy for set to make iteration safe (not doing so leads to smashed stacks and tears!)
    // TODO/idea: create list of objects to delete at end of frame to avoid dangling refs?
    std::set<CSState::CSNode*> ns = *(std::set<CSState::CSNode*>*)hs->Value();

    Handle<Function> fn = Handle<Function>::Cast(args[0]);
    for(auto i=ns.begin(); i!=ns.end(); ++i) {
        Local<Object> nwrapped = WrapNode(*i);
        Local<Value> val[]= { nwrapped };

        fn->Call(s.e->st.v8ctx->Global(), 1, val);
    }

    return v8::Undefined();
}

Handle<Value> EdgeSetForeachCallback(const Arguments &args)
{
    if (args.Length()<1) return v8::Undefined();

    if(!args[0]->IsFunction()) return v8::Undefined();

    // hoping for a this pointer
    Handle<External> hs = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    // create local copy for set to make iteration safe (not doing so leads to smashed stacks and tears!)
    // TODO/idea: create list of objects to delete at end of frame to avoid dangling refs?
    std::set<CSState::CSEdge*> es = *(std::set<CSState::CSEdge*>*)hs->Value();

    Handle<Function> fn = Handle<Function>::Cast(args[0]);
    for(auto i=es.begin(); i!=es.end(); ++i) {
        Local<Object> ewrapped = WrapEdge(*i);
        Local<Value> val[]= { ewrapped };

        fn->Call(s.e->st.v8ctx->Global(), 1, val);
    }

    return v8::Undefined();
}

Handle<Value> EdgeForeachCallback(const Arguments &args)
{
    if (args.Length()<1) return v8::Undefined();

    if(!args[0]->IsFunction()) return v8::Undefined();

    Handle<External> he = Handle<External>::Cast(args.Holder()->GetInternalField(0));
    CSState::CSEdge *e = (CSState::CSEdge*)he->Value();

    Handle<Function> fn = Handle<Function>::Cast(args[0]);

    Local<Object> n1wrapped = WrapNode(e->n1);
    Local<Value> val1[]= { n1wrapped };

    fn->Call(s.e->st.v8ctx->Global(), 1, val1);

    Local<Object> n2wrapped = WrapNode(e->n2);
    Local<Value> val2[]= { n2wrapped };

    fn->Call(s.e->st.v8ctx->Global(), 1, val2);

    return v8::Undefined();
}

void CSEngine::Init(CSMainWindow *wndw)
{
	wnd=wndw;
	#ifndef WIN32
	pthread_mutex_init(&loading_mutex,NULL);
	pthread_cond_init(&loading_cond,NULL);
	#else
	loading_mutex=PTHREAD_MUTEX_INITIALIZER;
	loading_cond=PTHREAD_COND_INITIALIZER;
    #endif

	graphics.Init(this);

	memset(keys,0,sizeof(keys));

	//s.s->PlayBGM();

	df1=14000000;
	df2=14000000;

	/* script editor defaults */
	editor_index=-1;
    editor_local=false;

    term_backlog_pos=-1;

    /* v8 engine */
    global = Persistent<ObjectTemplate>::New(ObjectTemplate::New());
    PropertyAttribute flags = (PropertyAttribute)(ReadOnly|DontDelete);
    PropertyAttribute flags2 = (PropertyAttribute)(flags|DontEnum);
    global->Set(String::New("_nat_node"), FunctionTemplate::New(NatNodeCallback), flags2);
    global->Set(String::New("_nat_edge"), FunctionTemplate::New(NatEdgeCallback), flags2);
    global->Set(String::New("_snap_heap"), FunctionTemplate::New(SnapHeapCallback), flags2);
    global->Set(String::New("rand"), FunctionTemplate::New(RandomCallback), flags);
    global->Set(String::New("countbits"), FunctionTemplate::New(CountbitsCallback), flags);
    global->Set(String::New("print"), FunctionTemplate::New(PrintCallback), flags);
    global->Set(String::New("println"), FunctionTemplate::New(PrintlnCallback), flags);
    global->Set(String::New("addNode"), FunctionTemplate::New(AddNodeCallback), flags);
    global->Set(String::New("delNode"), FunctionTemplate::New(DelNodeCallback), flags);
    global->Set(String::New("addEdge"), FunctionTemplate::New(AddEdgeCallback), flags);
    global->Set(String::New("delEdge"), FunctionTemplate::New(DelEdgeCallback), flags);
    global->Set(String::New("getEdge"), FunctionTemplate::New(GetEdgeCallback), flags);
    global->Set(String::New("gamma"), FunctionTemplate::New(GammaCallback), flags);
    global->Set(String::New("delta"), FunctionTemplate::New(DeltaCallback), flags);
    global->Set(String::New("nodes"), FunctionTemplate::New(NodesCallback), flags);
    global->Set(String::New("edges"), FunctionTemplate::New(EdgesCallback), flags);

    value_of_templ = Persistent<FunctionTemplate>::New(FunctionTemplate::New(ValueOfCallback));
    native_rendition_templ = Persistent<FunctionTemplate>::New(FunctionTemplate::New(NativeRenditionCallback));
    set_colour_templ = Persistent<FunctionTemplate>::New(FunctionTemplate::New(SetColourCallback));
    edge_foreach_templ = Persistent<FunctionTemplate>::New(FunctionTemplate::New(EdgeForeachCallback));

    /* global scripts; need to enter a context to compile */
    st.v8ctx = v8::Context::New(NULL, global);
    st.v8ctx->Enter();

    st.Clear();

	/* scripts */
	LoadScripts();

    HandleScope hslocal;
    Handle<ObjectTemplate> h = ObjectTemplate::New();
    h->SetInternalFieldCount(2);
    h->SetNamedPropertyHandler(NodeGet,NodeSet,0,0,NodeEnum);
    node_templ = Persistent<ObjectTemplate>::New(hslocal.Close(h));

    HandleScope hslocal2;
    h = ObjectTemplate::New();
    h->SetInternalFieldCount(2);
    h->Set(String::New("forEach"), FunctionTemplate::New(ForeachCallback));
    h->Set(String::New("size"), FunctionTemplate::New(SizeCallback));
    set_templ = Persistent<ObjectTemplate>::New(hslocal2.Close(h));

    HandleScope hslocal3;
    h = ObjectTemplate::New();
    h->SetInternalFieldCount(2);
    h->SetNamedPropertyHandler(EdgeGet,EdgeSet,0,0,EdgeEnum);
    edge_templ = Persistent<ObjectTemplate>::New(hslocal3.Close(h));

    HandleScope hslocal4;
    h = ObjectTemplate::New();
    h->SetInternalFieldCount(2);
    h->Set(String::New("forEach"), FunctionTemplate::New(EdgeSetForeachCallback));
    h->Set(String::New("size"), FunctionTemplate::New(EdgeSetSizeCallback));
    edge_set_templ = Persistent<ObjectTemplate>::New(hslocal4.Close(h));

    char buf[10240];
    FILE *fl=fopen("serialise.js","r");
    if(!fl) return;
    int len = fread(buf,1,10240,fl);
    buf[len]=0;
    EvalScript(buf,"serialise.js");
    fclose(fl);
}

void CSEngine::LoadScripts()
{
    scripts.clear();

    char buffn[128],buftype[128],buftitle[128],bufcode[4096];
    for(int i=0;;i++) {
        sprintf(buffn, "scripts/%d.js", i);
        FILE *fl=fopen(buffn, "r");
        if(!fl) break;

        scripts.push_back(CSScript());

        fscanf(fl,"// gd %[^ \n] script %[^\n] ",buftype,buftitle);
        if(!strcmp(buftype,"node")) scripts.back().onNodes=true;
        else scripts.back().onNodes=false;
        scripts.back().name = buftitle;
        int len = fread(bufcode,1,4096,fl);
        bufcode[len]=0;
        scripts.back().code = bufcode;

        fclose(fl);
    }
}

void CSEngine::SaveScripts()
{
    char buffn[128];
    for(int i=0;;i++) {
        sprintf(buffn, "scripts/%d.js", i);
        if(unlink(buffn)) break;
    }   
    for(int i=0;i<scripts.size();++i) {
        SaveScript(i);
    }
}

void CSEngine::SaveScript(int id)
{
    char buffn[128];
    sprintf(buffn, "scripts/%d.js", id);
    FILE *fl=fopen(buffn, "w");
    fprintf(fl,"// gd %s script %s\n", scripts[id].onNodes?"node":"graph", scripts[id].name.c_str());
    fprintf(fl,"%s",scripts[id].code.c_str());
    fclose(fl);
}

bool CSEngine::RunScriptForNode(CSState::CSNode *n, const char *code, const char *name)
{
    std::string wcode = std::string("(function (N) { ") + code + "\n})";

    HandleScope hslocal;
    Local<Value> nwrapped = WrapNode(n);

    TryCatch try_catch;

    Handle<Script> out;
    out = Script::Compile(String::New(wcode.c_str()),String::New(name));
    if(try_catch.HasCaught()) {
        String::Utf8Value error(try_catch.StackTrace());
        if(err_buf[0]) {
            err_buf[strlen(err_buf)+1]=0;
            err_buf[strlen(err_buf)]='\n';
        }
        strncpy(err_buf+strlen(err_buf),*error,4096-strlen(err_buf));
        return false;
    }

    Handle<Function> f = Handle<Function>::Cast(out->Run());
    f->Call(s.e->st.v8ctx->Global(), 1, &nwrapped);

    if(try_catch.HasCaught()) {
        String::Utf8Value error(try_catch.StackTrace());
        if(err_buf[0]) {
            err_buf[strlen(err_buf)+1]=0;
            err_buf[strlen(err_buf)]='\n';
        }
        strncpy(err_buf+strlen(err_buf),*error,4096-strlen(err_buf));
        return false;
    }
    return true;
}

bool CSEngine::RunScriptForNodes(std::set<CSState::CSNode *> ns, const char *code, const char *name)
{
    std::string wcode = std::string("(function (N) { ") + code + "\n})";

    HandleScope hslocal;

    TryCatch try_catch;

    Handle<Script> out;
    out = Script::Compile(String::New(wcode.c_str()),String::New(name));
    if(try_catch.HasCaught()) {
        String::Utf8Value error(try_catch.StackTrace());
        if(err_buf[0]) {
            err_buf[strlen(err_buf)+1]=0;
            err_buf[strlen(err_buf)]='\n';
        }
        strncpy(err_buf+strlen(err_buf),*error,4096-strlen(err_buf));
        return false;
    }

    Handle<Function> f = Handle<Function>::Cast(out->Run());

    for(auto &n: ns) {
        if(!n->selected) continue;

        Local<Value> nwrapped = WrapNode(n);
        f->Call(s.e->st.v8ctx->Global(), 1, &nwrapped);

        if(try_catch.HasCaught()) {
            String::Utf8Value error(try_catch.StackTrace());
            if(err_buf[0]) {
                err_buf[strlen(err_buf)+1]=0;
                err_buf[strlen(err_buf)]='\n';
            }
            strncpy(err_buf+strlen(err_buf),*error,4096-strlen(err_buf));
            return false;
        }
    }

    return true;
}

bool CSEngine::RunScript(const char *code, const char *name)
{
    std::string wcode = std::string("(function () { ") + code + "\n})";

    HandleScope hslocal;

    TryCatch try_catch;

    Handle<Script> out;
    out = Script::Compile(String::New(wcode.c_str()),String::New(name));
    if(try_catch.HasCaught()) {
        String::Utf8Value error(try_catch.StackTrace());
        if(err_buf[0]) {
            err_buf[strlen(err_buf)+1]=0;
            err_buf[strlen(err_buf)]='\n';
        }
        strncpy(err_buf+strlen(err_buf),*error,4096-strlen(err_buf));
        return false;
    }

    Handle<Function> f = Handle<Function>::Cast(out->Run());
    f->Call(s.e->st.v8ctx->Global(), 0, NULL);

    if(try_catch.HasCaught()) {
        String::Utf8Value error(try_catch.StackTrace());
        if(err_buf[0]) {
            err_buf[strlen(err_buf)+1]=0;
            err_buf[strlen(err_buf)]='\n';
        }
        strncpy(err_buf+strlen(err_buf),*error,4096-strlen(err_buf));
        return false;
    }
    return true;
}

std::string CSEngine::EvalScript(const char* code, const char *name)
{
    TryCatch try_catch;
    Local<Script> out;
    out = Script::Compile(String::New(code),String::New(name));
    if(try_catch.HasCaught()) {
        String::Utf8Value error(try_catch.StackTrace());
        std::string errstr = std::string("[error] ") + *error;
        return errstr;
    }

    Local<Value> val = out->Run();
    if(try_catch.HasCaught()) {
        String::Utf8Value error(try_catch.StackTrace());
        std::string errstr = std::string("[error] ") + *error;
        return errstr;
    }
    return ToJSON(val);
}

void *LolThread(void *data)
{
	CSEngine *d=(CSEngine*)data;
	while(1) {
	    d->RunLogic();
	    timespec ts;
        ts.tv_sec=0; //ts.tv_nsec=16000000;//12000000;
        ts.tv_nsec=((CSEngine*)data)->df2;
        pthread_delay_np(&ts);
	}
	return 0;
}

void *fpscount(void *data)
{
	while(1)
	{
		timespec ts;
		ts.tv_sec=1; ts.tv_nsec=0;
		pthread_delay_np(&ts);

		s.e->sf1=s.e->f1; if(s.e->sf1<40) s.e->df1-=300000; else if(s.e->sf1>67) s.e->df1+=300000;
		s.e->sf2=s.e->f2; if(s.e->sf2<56) s.e->df2-=300000; else if(s.e->sf2>67) s.e->df2+=300000;
		s.e->f1=s.e->f2=0;

		if(s.e->df1<5000000) s.e->df1=5000000;

		char buf[64];
		sprintf(buf,"rpg g=%d/l=%d/delta-t_g=%d/delta-t_l=%d",s.e->sf1,s.e->sf2,s.e->df1,s.e->df2);
		s.e->tdelta=0;
		/*#ifdef WIN32
		SetWindowText(s.wnd->GetHWND(),buf);
		#else
		XSetStandardProperties( s.wnd->g_pDisplay,
                            s.wnd->g_window,
                            buf,
                            "rpg",
                            None,
                            0,
                            NULL,
                            NULL );
		#endif*/
	}
}


void CSEngine::Click(int btn, int x,int y)
{
    if(btn==3) {
        printf("opening popup\n");
        ImGui::OpenPopupEx("context",true);
        graphics.sx0= graphics.mx;
        graphics.sy0= graphics.my;
    }

    if(action==AC_NOTHING && (btn==1 || ( !st.nselected && btn==3) )  )  {
        // select?
        if(!keys[KEY_SHIFT]) {
            st.nselected=0;
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                (*i)->selected=false;
            }
        }
        if( CSState::CSNode *v = GetClosestVertex(graphics.mx, graphics.my, 20.0f) ) {
            if(!v->selected) st.nselected += 1;
            v->selected = true;
        }
    } else if(action==AC_EDGE && btn==1) {
        // commit edge, but don't add more
        CSState::CSNode *v;
        if( v = GetClosestVertex(graphics.mx, graphics.my, 20.0f) ) {
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                if((*i)->selected) {
                    st.AddEdge(v,*i);
                }
            }
        }
        action = AC_NOTHING;
    }

    if(btn==4) {
        graphics.zoom /= 1.1;
    } else if(btn==5) {
        graphics.zoom *= 1.1;
    }
}

void CSEngine::StartDragging(int x0, int y0)
{
    if(!graphics.dragging1) {
        graphics.sx0=x0;
        graphics.sy0=y0;
        graphics.dragging1=true;
        graphics.sx1=graphics.mx;
        graphics.sy1=graphics.my;
    }
}

void CSEngine::CancelDragging()
{
    if(graphics.dragging1) {
        graphics.dragging1=false;
        action=AC_NOTHING;
    }
}

void CSEngine::SetSelectionRect(int x0, int y0, int x1, int y1)
{
    st.nselected=0;
    for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
        float sx, sy;
        graphics.SpaceToScreen( (*i)->pos[0], (*i)->pos[1], (*i)->pos[2], sx, sy);

        //ImGui::Text("%.2f %.2f",sx,sy);
        if(  ((sx>=x0 && sx<=x1) || (sx<=x0 && sx>=x1) )
           &&((sy>=y0 && sy<=y1) || (sy<=y0 && sy>=y1) ) ) {
           ++st.nselected;
            (*i)->selected=true;
        } else {
            (*i)->selected=false;
        }
    }
    graphics.sx0=x0;
    graphics.sy0=y0;
    graphics.sx1=x1;
    graphics.sy1=y1;
}

CSState::CSNode* CSEngine::GetClosestVertex(int x, int y, float rmax)
{
    float dmin=rmax*rmax;
    CSState::CSNode *closest = NULL;

    for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
        float sx, sy;
        graphics.SpaceToScreen( (*i)->pos[0], (*i)->pos[1], (*i)->pos[2], sx, sy);

        float d = (sx-x)*(sx-x) + (sy-y)*(sy-y);

        if(d<dmin) {
            closest = *i;
            dmin = d;
        }
    }

    return closest;
}

bool CSEngine::ScriptListEntry(CSScript *s, int id, bool local)
{
    bool reassign=false;
    if(!local) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,0,1));
    if(ImGui::Selectable(s->name.c_str())) {
        editor_index=id;
        editor_local=local;
        strcpy(editor_tbuf,s->name.c_str());
        strcpy(editor_buf,s->code.c_str());
        editor_is_pernode = s->onNodes;
    }
    if(!local) ImGui::PopStyleColor();
    if(ImGui::BeginPopupContextItem("local or global",2)) {
        if(ImGui::MenuItem("Delete")) {
            /* close editor if this script was open */
            if(editor_local == local && editor_index == id) editor_index = -1;
            /* erase script, possibly sync on disk */
            if(local) st.scripts.erase(st.scripts.begin()+id);
            else {
                scripts.erase(scripts.begin()+id);
                SaveScripts();
            }
            return false;
        }
        if(ImGui::MenuItem("Store with graph","",&local)) {
            reassign=true;
        }
        ImGui::EndPopup();
    }

    ImGui::NextColumn();
    if(!s->onNodes) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55,0.1,0.2,1.0));
    else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2,0.5,0.2,1.0));
    if(ImGui::Button("Run")) {
        if(s->onNodes) {
            RunScriptForNodes(st.nodes, s->code.c_str(), s->name.c_str());
        } else {
            RunScript(s->code.c_str(), s->name.c_str());
        }
     }
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    if(reassign) {
        if(local) {
            st.scripts.push_back(*s);
            scripts.erase(scripts.begin()+id);
            SaveScripts();

            /* if it was open in the editor, repoint */
            if(editor_index==id && editor_local==!local) {
                editor_local=local;
                editor_index=st.scripts.size()-1;
            }
        } else {
            scripts.push_back(*s);
            st.scripts.erase(st.scripts.begin()+id);
            SaveScripts();

            /* if it was open in the editor, repoint */
            if(editor_index==id && editor_local==!local) {
                editor_local=local;
                editor_index=scripts.size()-1;
            }
        }
        return false;
    }
    return true;
}

void CSEngine::RenderObjectProps(Handle<Object> obj, bool show_builtins)
{
    ImGui::PushID(obj->GetIdentityHash());

    HandleScope scope;
    Handle<Array> props = obj->GetPropertyNames();
    for(int i=0;i<props->Length();++i) {
        ImGui::PushID(i);
        ImGui::AlignFirstTextHeightToWidgets();
        
        Handle<Value> pname = props->Get(i);
        Handle<Value> pvalue = obj->Get(pname);
        
        RenderNamedProperty(*String::Utf8Value(pname), pvalue, show_builtins);

        ImGui::PopID();
    }
    ImGui::PopID();
}

void CSEngine::RenderAttrs(CSState::attrs *a, bool show_builtins)
{
    ImGui::PushID(a);
    int n=0;
    for(auto i = a->begin(); i!= a->end(); ++i) {
        ++n;
        ImGui::PushID(n);
        ImGui::AlignFirstTextHeightToWidgets();
        
        RenderNamedProperty(i->first.c_str(), i->second.j_data, show_builtins);

        ImGui::PopID();
    }
    ImGui::PopID();
}

void CSEngine::RenderNamedProperty(const char *name, v8::Handle<v8::Value> pvalue, bool show_builtins)
{
    HandleScope scope;
    if(pvalue->IsObject()) {
        Handle<Object> pobj = pvalue->ToObject();
        if(pvalue->IsFunction()) {
            if(pobj.As<Function>()->GetScriptLineNumber() == Function::kLineOffsetNotFound) {
                if(show_builtins) {
                    ImGui::Bullet();
                    ImGui::Selectable(name);
                    ImGui::NextColumn();
                    ImGui::Text("builtin function");
                    ImGui::NextColumn();
                }
                return;
            }
        }
        bool node_open = ImGui::TreeNode("##object",name);
        ImGui::NextColumn();
        if(pobj->InternalFieldCount() > 1) {
            /* native object */
            int notype = pobj->GetInternalField(1)->Int32Value();
            ImGui::Text("%s",nonames[notype]);
            ImGui::NextColumn();

            if(node_open) {
                Handle<External> hn = Handle<External>::Cast(pobj->GetInternalField(0));
    
                switch(notype) {
                case NO_NODE:
                    CSState::CSNode *n; n = (CSState::CSNode*)hn->Value();
                    RenderAttrs(&n->a, show_builtins);
                    break;
                case NO_EDGE:
                    CSState::CSEdge *e; e = (CSState::CSEdge*)hn->Value();
                    RenderAttrs(&e->a, show_builtins);
                    break;
                }
                ImGui::TreePop();
            }
        } else {
            /* javascript object */
            String::Utf8Value v(pvalue);
            ImGui::Text("%s",*v);
            ImGui::NextColumn();
            if(node_open) {
                RenderObjectProps(pobj, show_builtins);
                ImGui::TreePop();
            }
        }
    } else {
        ImGui::Bullet();
        ImGui::Selectable(name);
        ImGui::NextColumn();
        ImGui::Text("%s",*String::Utf8Value(pvalue));
        //ImGui::InputText("##value",*String::Utf8Value(pvalue),32);
        ImGui::NextColumn();
    }
}

void CSEngine::AcEdge(float x, float y)
{
    CSState::CSNode* v;
    if(action == AC_EDGE) {
        if( v = GetClosestVertex(x, y, 20.0f) ) {
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                if((*i)->selected) {
                    st.AddEdge(v,*i);
                }
                (*i)->selected = false;
            }
            v->selected = true;
            action = AC_EDGE;
        } else action = AC_NOTHING;
    } else {
        if( v = GetClosestVertex(x, y, 20.0f) ) {
            st.nselected=1;
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                (*i)->selected=false;
            }
            v->selected = true;
            action = AC_EDGE;
        }
    }
}

void CSEngine::RunLogic()
{
    ++f2;
    int w;

    /* Keys, buttons */

    /* RO-style view angle update */
    graphics.angle -= graphics.dangle;
    graphics.vangle +=graphics.dvangle;
    graphics.zoom +=graphics.dzoom;

    if(graphics.angle > PI) graphics.angle -= 2*PI;
    if(graphics.angle < -PI) graphics.angle += 2*PI;
    if(graphics.vangle > (PI/2.0-0.0001)) graphics.vangle = PI/2.0-0.0001;
    if(graphics.vangle < (-PI/2.0+0.0001)) graphics.vangle = -PI/2.0+0.0001;

    /* movement speed decay */
    graphics.dangle *= 0.82f;
    graphics.dvangle *= 0.82f;
    graphics.dzoom *= 0.82f;

    if(keys[KEY_ESC]) {
        keys[KEY_ESC]=false;
        action=AC_NOTHING;
    } else if(keys[KEY_B]) {
        keys[KEY_B]=false;
        action=AC_SELECT;
        graphics.sx0=graphics.mx;
        graphics.sy0=graphics.my;
        graphics.dragging1=true;
        graphics.sx1=graphics.mx;
        graphics.sy1=graphics.my;
    } else if(keys[KEY_G]) {
        keys[KEY_G]=false;
        action=AC_GO;
        graphics.sx0=graphics.mx;
        graphics.sy0=graphics.my;
        graphics.dragging1=true;
        graphics.sx1=graphics.mx;
        graphics.sy1=graphics.my;
    } else if(keys[KEY_A]) {
        keys[KEY_A]=false;
        if(st.nselected == 0) {
            st.nselected = st.nodes.size();
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                (*i)->selected=true;
            }
        } else {
            st.nselected=0;
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                (*i)->selected=false;
            }
        }
    } else if(keys[KEY_Q]) {
        action=AC_RELAX;
    } else if(keys[KEY_M]) {
        if(st.nselected) {
            float sx=0,sy=0,sz=0;
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                if( (*i)->selected ) {
                    sx += (*i)->pos[0];
                    sy += (*i)->pos[1];
                    sz += (*i)->pos[2];
                }
            }
            sx *= 1.0f/st.nselected;
            sy *= 1.0f/st.nselected;
            sz *= 1.0f/st.nselected;

            if(!keys[KEY_SHIFT]) {
                graphics.tx=sx;
                graphics.ty=sy;
                graphics.tz=sz;
            } else {
                for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                    if( (*i)->selected ) {
                        (*i)->pos[0] += graphics.tx-sx;
                        (*i)->pos[1] += graphics.ty-sy;
                        (*i)->pos[2] += graphics.tz-sz;
                    }
                }
            }
        }
    } else if(keys[KEY_TAB]) {
        keys[KEY_TAB]=false;
        show_terminal = true;
        force_terminal_focus = true;
    }

    /* Relax layout energy */
    if(action == AC_RELAX) {
        if(!st.RelaxationStep(0.1f)) action=AC_NOTHING;
    }

    /* RMB drag: rotate */
    if(keys[3] && use_3d) {
         if(!graphics.dragging3) {
            graphics.sx0=graphics.mx;
            graphics.sy0=graphics.my;
        }
        graphics.dragging3=true;
        graphics.dangle *= 0.5;
        graphics.dvangle *= 0.5;
        graphics.dangle += 0.5* ( (graphics.mx-graphics.sx0)*0.02f );
        graphics.dvangle += 0.5* ( (graphics.my-graphics.sy0)*0.01f );
        graphics.sx0=graphics.mx;
        graphics.sy0=graphics.my;
    } else graphics.dragging3=false;

    /* MMB drag: translate */
    if(keys[2]) {
         if(!graphics.dragging2) {
            graphics.sx0=graphics.mx;
            graphics.sy0=graphics.my;
        }
        graphics.dragging2=true;
        float gx0,gy0,gz0,gx1,gy1,gz1;
        graphics.ScreenToSpace(graphics.sx0,graphics.sy0,gx0,gy0,gz0);
        graphics.ScreenToSpace(graphics.mx,graphics.my,gx1,gy1,gz1);
        graphics.tx -= gx1-gx0;
        graphics.ty -= gy1-gy0;
        graphics.tz -= gz1-gz0;

        graphics.sx0=graphics.mx;
        graphics.sy0=graphics.my;
    } else graphics.dragging2=false;

    /* LMB drag actions */
    if(graphics.dragging1 && action==AC_SELECT) {
        SetSelectionRect(graphics.sx0,graphics.sy0, graphics.mx, graphics.my);
    } else if(graphics.dragging1 && action==AC_GO) {
        float gx0,gy0,gz0,gx1,gy1,gz1;
        graphics.ScreenToSpace(graphics.sx0,graphics.sy0,gx0,gy0,gz0);
        graphics.ScreenToSpace(graphics.mx,graphics.my,gx1,gy1,gz1);
        for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
            if((*i)->selected) {
                (*i)->pos[0] += gx1-gx0;
                (*i)->pos[1] += gy1-gy0;
                (*i)->pos[2] += gz1-gz0;
            }
        }
        graphics.sx0=graphics.mx;
        graphics.sy0=graphics.my;
    } else if(graphics.dragging1 && action==AC_SCALE) {
    }

    /* move with arrow keys */
    float kdx=0.0f, kdy=0.0f;
    if(keys[KEY_UP]) kdy=4.f;
    else if(keys[KEY_DOWN]) kdy=-4.f;
    if(keys[KEY_LEFT]) kdx=4.f;
    else if(keys[KEY_RIGHT]) kdx=-4.f;
    if(kdx!=0.0f||kdy!=0.0f) {
        float gx0,gy0,gz0,gx1,gy1,gz1;
        graphics.ScreenToSpace(0.0f,0.0f,gx0,gy0,gz0);
        graphics.ScreenToSpace(kdx,kdy,gx1,gy1,gz1);

        graphics.tx -= gx1-gx0;
        graphics.ty -= gy1-gy0;
        graphics.tz -= gz1-gz0;
    }

    if(keys[KEY_V]) {
        float x,y,z;
        graphics.ScreenToSpace(graphics.mx, graphics.my,x, y,z);
        keys[KEY_V]=false;
        CSState::CSNode *n = st.AddNode( x,y,z );
        n->selected = true;
        ++st.nselected;    } else if(keys[KEY_C]) {
        float x,y,z;
        graphics.ScreenToSpace(graphics.mx, graphics.my,x, y,z);
        keys[KEY_C]=false;
        for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
            for(auto j=st.nodes.begin(); j!=st.nodes.end(); ++j) {
                if(i!=j && (*i)->selected && (*j)->selected) {
                    st.AddEdge(*i,*j);
                }
            }
        }
    } else if(keys[KEY_D]) {
        float x,y,z;
        graphics.ScreenToSpace(graphics.mx, graphics.my,x, y,z);
        keys[KEY_D]=false;
        if(keys[KEY_SHIFT]) {
            // shift-d: duplicate
            st.DuplicateSelection();
            action=AC_GO;
            graphics.sx0=graphics.mx;
            graphics.sy0=graphics.my;
            graphics.dragging1=true;
            graphics.sx1=graphics.mx;
            graphics.sy1=graphics.my;
        } else {
            // d: disconnect
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                for(auto j=st.nodes.begin(); j!=st.nodes.end(); ++j) {
                    if(i!=j && (*i)->selected && (*j)->selected) {
                        st.DelEdge(*i,*j);
                    }
                }
            }
        }
    } else if(keys[KEY_X]) {
        keys[KEY_X]=false;
        st.ExtrudeSelection();
        action=AC_GO;
        graphics.sx0=graphics.mx;
        graphics.sy0=graphics.my;
        graphics.dragging1=true;
        graphics.sx1=graphics.mx;
        graphics.sy1=graphics.my;
    } else if(keys[KEY_S]) {
        keys[KEY_S]=false;
        for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
            for(auto j=st.nodes.begin(); j!=st.nodes.end(); ++j) {
                if(i!=j && (*i)->selected && !(*j)->selected) {
                    st.DelEdge(*i,*j);
                }
            }
        }
    } else if(keys[KEY_E]) {
        keys[KEY_E]=false;
        CSState::CSNode *v;
        AcEdge(graphics.mx, graphics.my);
    } else if(keys[KEY_DELETE]) {
        float x,y,z;
        action = AC_NOTHING;
        graphics.ScreenToSpace(graphics.mx, graphics.my,x, y,z);
        keys[KEY_DELETE]=false;
        for(auto i=st.nodes.begin(); i!=st.nodes.end(); ) {
            if( (*i)->selected ) {
                auto j = i;
                ++i;
                st.DelNode(*j);
            } else ++i;
        }
    }

    /* selection groups */
    for(int i=0;i<NSELGROUPS;++i) {
        if(keys[KEY_GROUP_ACCEL[i]]) {
            keys[KEY_GROUP_ACCEL[i]]=false;
            if(keys[KEY_CTRL])
                st.SaveSelectionGroup(i);
            else
                st.LoadSelectionGroup(i);
        }
    }

    /* GUI */
    static bool rs_every_node=false;

    if(ImGui::BeginPopup("context")) {
        float x,y,z;
        graphics.ScreenToSpace(graphics.sx0, graphics.sy0,x, y,z);
        ImGui::TextColored(ImVec4(.5,.5,.5,1.0),"%.2f %.2f %.2f",x,y,z);

        /* node data display */
        if( CSState::CSNode* v = GetClosestVertex(graphics.sx0, graphics.sy0, 20.0f) ) {
            ImGui::SetNextWindowContentWidth(200.0f);
            if(ImGui::BeginMenu("Vertex data")) {
                ImGui::Columns(2);
                ImGui::SetColumnOffset(1,70.0f);
                ImGui::Text("pos.x"); ImGui::NextColumn(); ImGui::Text("%.2f",v->pos[0]); ImGui::NextColumn();
                ImGui::Text("pos.y"); ImGui::NextColumn(); ImGui::Text("%.2f",v->pos[1]); ImGui::NextColumn();
                ImGui::Text("pos.z"); ImGui::NextColumn(); ImGui::Text("%.2f",v->pos[2]); ImGui::NextColumn();

                RenderAttrs(&v->a, false);

                ImGui::Columns(1);
                ImGui::EndMenu();
            }
            ImGui::Separator();
        }

        if(ImGui::MenuItem("Add node","V")) {
            CSState::CSNode *n = st.AddNode( x,y,z );
            n->selected = true;
            ++st.nselected;
        }
        if(ImGui::MenuItem("Connect nodes","C")) {
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                for(auto j=st.nodes.begin(); j!=st.nodes.end(); ++j) {
                    if(i!=j && (*i)->selected && (*j)->selected) {
                        st.AddEdge(*i,*j);
                    }
                }
            }
        }
        if(ImGui::MenuItem("Add edge","E")) {
            AcEdge(graphics.sx0,graphics.sy0);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Extrude selection","X")) {
            st.ExtrudeSelection();
            action=AC_GO;
            graphics.sx0=graphics.mx;
            graphics.sy0=graphics.my;
            graphics.dragging1=true;
            graphics.sx1=graphics.mx;
            graphics.sy1=graphics.my;
        }

        ImGui::Separator();
        if(ImGui::MenuItem("Delete nodes","Del")) {
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ) {
                if( (*i)->selected ) {
                    auto j = i;
                    ++i;
                    st.DelNode(*j);
                } else ++i;
            }
        }
        if(ImGui::MenuItem("Disconnect nodes","D")) {
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                for(auto j=st.nodes.begin(); j!=st.nodes.end(); ++j) {
                    if(i!=j && (*i)->selected && (*j)->selected) {
                        st.DelEdge(*i,*j);
                    }
                }
            }
        }
        if(ImGui::MenuItem("Separate nodes","S")) {
            for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                for(auto j=st.nodes.begin(); j!=st.nodes.end(); ++j) {
                    if(i!=j && (*i)->selected && !(*j)->selected) {
                        st.DelEdge(*i,*j);
                    }
                }
            }
        }
        ImGui::Separator();
        if(ImGui::BeginMenu("Selection")) {
            if(ImGui::MenuItem("Shrink")) {
                st.ShrinkSelection();
            }
            if(ImGui::MenuItem("Grow")) {
                st.GrowSelection();
            }
            if(ImGui::MenuItem("Grow to component")) {
                st.SelectConnected();
            }
            if(ImGui::MenuItem("Invert")) {
                st.InvertSelection();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if(ImGui::MenuItem("Execute on selection...")) {
            rs_every_node=true;
        }
        if(ImGui::BeginMenu("Execute again")) {
            for(auto j=recent.begin(); j!=recent.end(); ++j) {
                if(ImGui::MenuItem( (j->substr(0,8)+(j->length()>8?"...":"")).c_str() )) {
                    RunScriptForNodes(st.nodes, j->c_str(), "[temp]");
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    /* Main status */
    ImGui::SetNextWindowPos(ImVec2(130,30),ImGuiSetCond_FirstUseEver);
    ImGui::Begin("View", NULL, ImGuiWindowFlags_ShowBorders);
    ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;

    ImGui::Text("Center: %.2f %.2f %.2f", graphics.tx, graphics.ty, graphics.tz);
    ImGui::Text("H Angle: %.1f°", graphics.angle*180/PI);
    ImGui::Text("V Angle: %.1f°", graphics.vangle*180/PI);

    if(ImGui::Button("Reset")) {
        graphics.ResetView();    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(5,30),ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Graph", NULL, ImGuiWindowFlags_ShowBorders);
    ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;

    if(!st.nselected) ImGui::Text("Vertices: %d   ", st.nodes.size());
    else ImGui::Text("Vertices: %d/%d", st.nselected, st.nodes.size());
    ImGui::Text("Edges: %d", st.edges.size());

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(300,30),ImGuiSetCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(3,1));
    ImGui::Begin("Selection Groups",NULL,ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders);
    ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(1,1));
    for(int i=0;i<NSELGROUPS;++i) {
        if(st.selgroups[i].size()) {
            char buf[16]; sprintf(buf,"%d##sg%d",st.selgroups[i].size(),i);
            if(ImGui::Button(buf,ImVec2(32,32))) {
                st.LoadSelectionGroup(i);
            }
        } else ImGui::Button("-",ImVec2(32,32));
        ImGui::SameLine();
    }
    ImGui::PopStyleVar();

    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::SetNextWindowPos(ImVec2(5,150),ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Node Appearance",NULL,ImGuiWindowFlags_ShowBorders);
    ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;

    ImGui::PushItemWidth(40.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(1,1));
    for(int i=0;i<9;++i) {
        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Text, *(ImVec4*)graphics.nodecol[i]);
        ImGui::InputText("##nl",graphics.nodelook[i],32);
        ImGui::PopStyleColor();
        if(ImGui::BeginPopupContextItem("label color",2)) {
            ImGui::Text("Label Color:");
            ImGui::ColorEdit4("##edit",graphics.nodecol[i],true);
            ImGui::EndPopup();
        }
        if( (i%3)<2) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();

    ImGui::End();

    /* scripts window */
    ImGui::SetNextWindowPos(ImVec2(5,250),ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200,150),ImGuiSetCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(10,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4,8));
    ImGui::Begin("Scripts",NULL,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ShowBorders);
    ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;

    ImGui::BeginChild("ScrollingRegion", ImVec2(-1,-ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::Columns(2,"##scolumns",false);
    ImGui::SetColumnOffset(1,ImGui::GetWindowContentRegionWidth()-50);

    for(int i=0;i<scripts.size();) {
        ImGui::PushID(i);
        i+=ScriptListEntry( &scripts[i], i, false );
        ImGui::PopID();
    }

    for(int i=0;i<st.scripts.size();) {
        ImGui::PushID(10000+i);
        i+=ScriptListEntry( &st.scripts[i], i, true );
        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Separator();

    if(ImGui::Button("Add")) {
        st.scripts.push_back( {"New Script","",true} );
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    /* javascript object browser */
    ImGui::SetNextWindowPos(ImVec2(5,450),ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200,150),ImGuiSetCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(10,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4,8));
    if(ImGui::Begin("Data",NULL,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ShowBorders)) {
    ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;

    ImGui::BeginChild("ScrollingRegion", ImVec2(-1,-ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::Columns(2,"##scolumns");

    static bool show_builtins = false;
    RenderObjectProps(st.v8ctx->Global(),show_builtins);

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::Checkbox("Show builtins",&show_builtins);
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    /* top menu bar */
    ImGui::BeginMainMenuBar();

    if(ImGui::BeginMenu("File")) {
        if(ImGui::MenuItem("New")) {
            st.Clear();
            wnd->SetWindowTitle("[new] - graphic depictions");
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Load...")) {
            st.Load();
        }
        if(ImGui::MenuItem("Save")) {
            st.Save();
        }
        if(ImGui::MenuItem("Save as...")) {
            st.SaveAs();
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Export to tikz...")) {
            char *filename=wnd->GetFilename("Export",false);
            if(filename && filename[0]) {
                ExportTikz(&st,filename);
            }
            if(filename) free(filename);        }
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Generate")) {
        if(ImGui::BeginMenu("Discrete Cube")) {
            static int dim=3;
            ImGui::SliderInt("##Dimension",&dim,1,6,"%.0fD");
            if(ImGui::MenuItem("Generate")) st.MkDiscreteCube(dim);
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Erdös-Renyi")) {
            static int n=3;
            static float p=0.5f;
            ImGui::DragInt("##ern",&n,1,1,100,"n = %.0f");
            ImGui::SliderFloat("##erp",&p,0.0f, 1.0f, "p = %.3f");
            if(ImGui::MenuItem("Generate")) st.MkGnp(n,p);
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Layout")) {
        if(ImGui::BeginMenu("Squash to 2D", use_3d)) {
            if(ImGui::MenuItem("To Z=0")) {
                graphics.angle=0.0f;
                graphics.vangle=0.0f;
                for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                    (*i)->pos[2]=0.0f;
                }
                use_3d=false;
            }
            if(ImGui::MenuItem("From view")) {
                graphics.angle=0.0f;
                graphics.vangle=0.0f;
                graphics.tx=0.0f;
                graphics.ty=0.0f;
                graphics.tz=0.0f;
                graphics.zoom=1.0f;
                for(auto i=st.nodes.begin(); i!=st.nodes.end(); ++i) {
                    float x,y;
                    graphics.SpaceToScreen((*i)->pos[0],(*i)->pos[1],(*i)->pos[2],x,y);
                    (*i)->pos[0]=(x/s.wnd->w)-0.5f;
                    (*i)->pos[1]=((y+(s.wnd->w-s.wnd->h)/2)/s.wnd->w)-0.5f;
                    (*i)->pos[2]=0.0f;
                }
                use_3d=false;
            }
            ImGui::EndMenu();
        }
        if(ImGui::MenuItem("Unlock 3D", "", false, !use_3d)) {
            use_3d=true;
            graphics.angle=0.0f;
            graphics.vangle=0.0f;
        }

        ImGui::Separator();
        if(ImGui::BeginMenu("Order by")) {
            if(ImGui::MenuItem("Distance from selection")) {
                st.OrderFromSelection();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Snap to grid")) {
            static float size=0.02;
            ImGui::SliderFloat("##GridSize",&size,0.001,0.1,"grid size = %.3f",2);
            if(ImGui::MenuItem("Snap")) st.RoundPositions(size);
            ImGui::EndMenu();
        }
        if(ImGui::MenuItem("Minimise energy","Q")) {
            action=AC_RELAX;        }

        ImGui::EndMenu();
    }

    static bool show_about=false;
    if(ImGui::BeginMenu("Help")) {
        if(ImGui::MenuItem("Intro")) {
        }
        if(ImGui::MenuItem("About")) {
            show_about=true;
        }
        ImGui::EndMenu();
    }

    switch(action) {
    default: break;
    case AC_GO: ImGui::MenuItem("Moving nodes.",NULL,false,false); break;
    case AC_SCALE: ImGui::MenuItem("Scaling nodes.",NULL,false,false); break;
    case AC_SELECT: ImGui::MenuItem("Selecting in rectangle.",NULL,false,false); break;
    case AC_RELAX: ImGui::MenuItem("Relaxing edges.",NULL,false,false); break;
    case AC_EDGE: ImGui::MenuItem("Adding an edge.",NULL,false,false); break;
    }

    ImGui::EndMainMenuBar();

    /* about box */
    if(show_about) {
        ImGui::OpenPopup("About graphic depictions");
        show_about=false;
    }
    ImGui::SetNextWindowSize(ImVec2(350,340),ImGuiSetCond_Appearing);
    if(ImGui::BeginPopupModal("About graphic depictions",NULL,ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ShowBorders)) {
        ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        p.x+=ImGui::GetWindowContentRegionWidth()/2;
        p.y+=15;
        draw_list->AddLine(ImVec2(p.x+16,p.y+32), ImVec2(p.x-16,p.y+32),0xFFFF8080,1);
        draw_list->AddLine(ImVec2(p.x+17,p.y+29), ImVec2(p.x+2,p.y+3),0xFFFF8080,1);
        draw_list->AddLine(ImVec2(p.x-17,p.y+29), ImVec2(p.x-2,p.y+3),0xFFFF8080,1);
        draw_list->AddCircle(p,4,0xFFFFFFFF,20,1);
        p.y+=32; p.x-=20;
        draw_list->AddCircle(p,5.5,0xFF0000FF,20,2);
        draw_list->AddCircle(p,4,0xFFFFFFFF,20,1);
        p.x+=40;
        draw_list->AddCircle(p,4,0xFFFFFFFF,20,1);
        ImGui::Dummy(ImVec2(0,60));
        #define CenteredText(c,t) { ImGui::SetCursorPosX( (ImGui::GetWindowContentRegionWidth()-ImGui::CalcTextSize(t).x)/2 ); ImGui::TextColored(c,t); }
        CenteredText(ImVec4(.7,.7,1,1),"graphic depictions "VERSTRING);
        CenteredText(ImVec4(1,1,1,1),"© 2016-2018 Matvey Soloviev\n<msoloviev@cs.cornell.edu>");
        ImGui::Separator();
        ImGui::Text("This program is licensed under the terms\nof the GNU General Public License version 3.\n\nAvailable online under:\nhttp://www.gnu.org/licenses/gpl-3.0.html");
        ImGui::Separator();
        ImGui::Text("Uses dear imgui\n (c) 2015, Omar Cornut and ImGui contributors.");
        ImGui::Text("Uses the V8 JavaScript engine \n (c) 2014, the V8 project authors.");
        ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth());
        if(ImGui::Button("OK",ImVec2(ImGui::GetWindowContentRegionWidth(),0))) {
            err_buf[0]=0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }

    /* popup terminal */
    if(show_terminal) {
        ImGuiContext& g = *GImGui;
        ImGui::SetNextWindowPos(ImVec2(0.0f, 2*g.IO.DisplaySize.y/3.0f));
        ImGui::SetNextWindowSize(ImVec2(g.IO.DisplaySize.x, g.IO.DisplaySize.y/3.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0,0));
        ImGui::Begin("##PopupTerminal", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings);

        ImVec2 input_size = ImVec2( g.IO.DisplaySize.x - 20.0f, 8.0f);
        int nlines=1;
        char *ptr = term_buf;
        while (char c = *ptr++) 
            if(c == '\n') 
                nlines++;
        input_size.y += g.FontSize*nlines;

        ImGui::BeginChild("ScrollingRegion", ImVec2(0,-5.0f-input_size.y), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear")) {
                term_backlog.clear();
                term_results.clear();
            }
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
        for (int i = 0; i < term_results.size(); i++) {
            const char *item = term_results[i].c_str();
            ImColor col;
            if (!strncmp(item, "> ",2)) col = ImColor(1.0f,1.0f,1.0f,1.0f);
            else if (!strncmp(item, "[error]",7)) col = ImColor(1.0f,0.4f,0.4f,1.0f);
            else if (!strncmp(item, ":",1)) { col = ImColor(0.7f,0.7f,1.0f,1.0f); ++item; }
            else col = ImColor(0.5f,1.0f,0.5f,1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(item);
            ImGui::PopStyleColor();
        }
//        if (ScrollToBottom)
        ImGui::SetScrollHere();
//        ScrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        // Command-line
        if (ImGui::InputTextEx("##input", term_buf, IM_ARRAYSIZE(term_buf), input_size, ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CtrlEnterForNewLine|ImGuiInputTextFlags_CallbackEscape|ImGuiInputTextFlags_CallbackHistory|ImGuiInputTextFlags_NoHorizontalScroll, 
    [] (ImGuiTextEditCallbackData* data) -> int {
        switch(data->EventFlag) {
        case ImGuiInputTextFlags_CallbackEscape:
            s.e->show_terminal=0;
            break;
        case ImGuiInputTextFlags_CallbackHistory: {
            const int prev_backlog_pos = s.e->term_backlog_pos;
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (s.e->term_backlog_pos == -1)
                    s.e->term_backlog_pos = s.e->term_backlog.size() - 1;
                else if (s.e->term_backlog_pos > 0)
                    s.e->term_backlog_pos--;
            } else if (data->EventKey == ImGuiKey_DownArrow) {
                if (s.e->term_backlog_pos != -1)
                    if (++s.e->term_backlog_pos >= s.e->term_backlog.size())
                        s.e->term_backlog_pos = -1;
            }

            if (prev_backlog_pos != s.e->term_backlog_pos)
            {
                data->CursorPos = data->SelectionStart = data->SelectionEnd = data->BufTextLen = (int)snprintf(data->Buf, (size_t)data->BufSize, "%s", (s.e->term_backlog_pos >= 0) ? s.e->term_backlog[s.e->term_backlog_pos].c_str() : "");
                data->BufDirty = true;
            }

            break; }
        }
        return 0;
    }, (void*)this)) {
            char* input_end = term_buf+strlen(term_buf);
            while (input_end > term_buf && input_end[-1] == ' ') input_end--; *input_end = 0;
            if (term_buf[0]) {
                term_backlog.push_back(term_buf);
                term_backlog_pos=-1;
                std::string res = EvalScript(term_buf,"[terminal input]");
                term_results.push_back(std::string("> ")+term_buf);
                term_results.push_back(res);
            }

            strcpy(term_buf, "");
            ImGui::SetKeyboardFocusHere(-1);
        } 

        if (force_terminal_focus || ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))) {
            ImGui::SetKeyboardFocusHere(-1); 
            force_terminal_focus= false;
        }
               
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    /* run script for selected nodes */
    ImGui::SetNextWindowSize(ImVec2(420,420),ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(200,200),ImGuiSetCond_FirstUseEver);
    if(rs_every_node && ImGui::Begin("Run script for selected nodes", NULL, ImGuiWindowFlags_ShowBorders)) {
        ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;
        static char buf[4096]={0};
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1));
        ImGui::BeginChild("ScrollingRegion", ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::InputTextMultiline("##source",buf,4096,ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
        ImGui::EndChild();
        if(ImGui::Button("Compile and Run")) {
            if(RunScriptForNodes(st.nodes,buf,"temp")) {
                // put into recent script deque
                std::string s(buf);
                recent.push_front(s);
                // remove duplicates
                for(auto i=recent.begin()+1; i!=recent.end(); ++i) {
                    if( *i == s ) {
                        recent.erase(i);
                        break;
                    }
                }
                // keep at most 5 items
                if(recent.size()>5) recent.pop_back();
            }
            rs_every_node=false;
        }
        ImGui::PopStyleVar();
        ImGui::End();
    }

    /* script editor */
    if(editor_index>=0) {
        static char title[128];
        sprintf(title,"%s - Script Editor###scripted", editor_tbuf);
        ImGui::SetNextWindowSize(ImVec2(400,400),ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(200,200),ImGuiSetCond_FirstUseEver);
        ImGui::Begin(title, NULL, ImGuiWindowFlags_ShowBorders);
        ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,2));
        ImGui::SameLine(9.0f);
        ImGui::InputText("##title",editor_tbuf,64);
        ImGui::SameLine(ImGui::GetWindowContentRegionWidth()-75.0f);

        ImGui::Checkbox("On nodes",&editor_is_pernode);

        if(ImGui::IsItemHovered()) {
            if(editor_is_pernode) ImGui::SetTooltip("Script is run once for each selected node.\nN is defined to refer to the current node.");
            else ImGui::SetTooltip("Script is run globally.\nUse nodes() to obtain set of all nodes.\nforEach(function (N) {...}) can be used to iterate.");
        }

        ImGui::Separator();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::InputTextMultiline("##source",editor_buf,4096,ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
        ImGui::EndChild();
        ImGui::Separator();
        if(ImGui::Button("Run")) {
            if(editor_is_pernode) {
                RunScriptForNodes(st.nodes, editor_buf,editor_tbuf);
            } else {
                RunScript(editor_buf, editor_tbuf);
            }
            rs_every_node=false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Save and Run")) {
            if(!editor_local) {
                scripts[editor_index].code = editor_buf;
                scripts[editor_index].name = editor_tbuf;
                scripts[editor_index].onNodes = editor_is_pernode;
                SaveScript(editor_index);
            } else {
                st.scripts[editor_index].code = editor_buf;
                st.scripts[editor_index].name = editor_tbuf;
                st.scripts[editor_index].onNodes = editor_is_pernode;
            }
            if(editor_is_pernode) {
                RunScriptForNodes(st.nodes, editor_buf,editor_tbuf);
            } else {
                RunScript(editor_buf, editor_tbuf);
            }
            rs_every_node=false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Save")) {
            if(!editor_local) {
                scripts[editor_index].code = editor_buf;
                scripts[editor_index].name = editor_tbuf;
                scripts[editor_index].onNodes = editor_is_pernode;
                SaveScript(editor_index);
            } else {
                st.scripts[editor_index].code = editor_buf;
                st.scripts[editor_index].name = editor_tbuf;
                st.scripts[editor_index].onNodes = editor_is_pernode;
            }
        }
        ImGui::SameLine();
        if(ImGui::Button("Close")) {
            editor_index=-1;
        }
        ImGui::PopStyleVar();
        ImGui::End();
    }

    /* script error popup */
    if(err_buf[0]) {
        ImGui::OpenPopup("Script Error");
        ImGui::SetNextWindowSize(ImVec2(300,100),ImGuiSetCond_Appearing);
        if(ImGui::BeginPopupModal("Script Error")) {
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x+ImGui::GetWindowContentRegionWidth());
            ImGui::Text(err_buf);
            ImGui::PopTextWrapPos();
            ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth());
            if(ImGui::Button("OK")) {
                err_buf[0]=0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopItemWidth();
            ImGui::EndPopup();
        }
    }



    //ImGui::Text( *String::Utf8Value(Script::Compile(String::New("'asdf'+3"),String::New("lol"))->Run()) );

    graphics.IncrementFrame();
}

void CSEngine::Run()
{
	pthread_t tid;
	//pthread_create(&tid,NULL,LolThread,(void*)this);
//	pthread_create(&tid,NULL,fpscount,(void*)this);
	long long t;
	int last_frame;

#ifndef WIN32
    timeval tv;

    gettimeofday(&tv,NULL);
    t=tv.tv_sec*1000000 + tv.tv_usec;
#else
	t = GetTickCount() * 1000LL;
#endif
    last_frame=t/(1000000LL/60);

	while(1)
	{
	    /* NEW SINGLETHREADED LOOP */
	    {
            ++f1;


    //		timeval tv1,tv2;
            if(f1==10) {
    //		    gettimeofday(&tv1,NULL);
                graphics.DoDraw();
    //          gettimeofday(&tv2,NULL);
    //          tdelta=tv2.tv_usec-tv1.tv_usec;
    //          if(tdelta<0) tdelta+=1000000;
            } else graphics.DoDraw();

            // must be here to make sure clicks -> engine -> graphics order is maintained
            wnd->CheckMessages();

            if(wnd->IsClosed()) return;

            /*timespec ts;
            ts.tv_sec=0; ts.tv_nsec=df1;//12000000;
            pthread_delay_np(&ts); */
	    }
#ifndef WIN32
		gettimeofday(&tv, NULL);
		t = tv.tv_sec * 1000000 + tv.tv_usec;
#else
		t = GetTickCount() * 1000LL;
#endif
        int fdelta=(t/(1000000LL/60))-last_frame;
        //for(int i=0;i<fdelta;++i) {
            RunLogic();
        //}
        if(!fdelta) {
            timespec ts;
            ts.tv_sec=0; //ts.tv_nsec=16000000;//12000000;
            ts.tv_nsec=df2;
            pthread_delay_np(&ts);
        }
        last_frame+=fdelta;
	}
}


